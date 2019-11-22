#pragma once


#include <memory>
#include <cassert>
#include <thread>
#include <message/message.hpp>
#include <boost/asio.hpp>
#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/exception/all.hpp>
#include <io/io_handler_initializer.hpp>
#include <io/io_streambuf.hpp>
#include <io/channel.hpp>


#define MAX_RECV_BUF_LEN       10240
#define MAX_SEND_BUF_LEN       10240
#define MAX_QUEUE_SIZE             10240


namespace micro
{
    namespace core
    {

        class tcp_channel : public channel, public std::enable_shared_from_this<tcp_channel>, public boost::noncopyable
        {
        public:

            typedef context_chain chain_type;
            typedef std::mutex mutex_type;

            typedef channel_source channel_type_id;
            typedef std::shared_ptr<io_streambuf> buf_ptr_type;

            typedef boost::asio::ip::tcp::socket socket_type;
            typedef boost::asio::ip::tcp::endpoint endpoint_type;

            typedef std::list<std::shared_ptr<message>> queue_type;
            typedef std::shared_ptr<boost::asio::io_service> ios_ptr_type;
            typedef std::shared_ptr<micro::core::io_handler_initializer> initializer_ptr_type;


            tcp_channel(ios_ptr_type ios, channel_type_id channel_id)
                : m_state(CHANNEL_INACTIVE)
                , m_channel_id(channel_id)
                , m_str_channel_id(m_channel_id.to_string())
                , m_ios(ios)
                , m_socket(*ios)
                , m_addr_info("addr info: UNKNOWN")
            {

            }

            virtual ~tcp_channel()
            {
                m_opts.clear();

                m_inbound_chain.clear();
                m_outbound_chain.clear();

                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_queue.clear();
                }
            }


            template<typename T>
            void option(std::string name, T value)
            {
                m_opts.option(name, value);
            }

            context_chain & inbound_pipeline() { return m_inbound_chain; }

            context_chain & outbound_pipeline() { return m_outbound_chain; }


            void channel_inbound_initializer(initializer_ptr_type io_handler_initializer)
            {
                if (nullptr == io_handler_initializer)
                {
                    return;
                }

                m_inbound_chain.set(IO_CONTEXT, this->shared_from_this());

                m_inbound_initializer = io_handler_initializer;
                m_inbound_initializer->init(m_inbound_chain);
            }

            void channel_outbound_initializer(initializer_ptr_type io_handler_initializer)
            {
                if (nullptr == io_handler_initializer)
                {
                    return;
                }

                m_outbound_chain.set(IO_CONTEXT, this->shared_from_this());

                m_outbound_initializer = io_handler_initializer;
                m_outbound_initializer->init(m_outbound_chain);
            }

            //get object in tcp channel

            virtual boost::asio::ip::tcp::socket & socket() { return m_socket; }

            virtual buf_ptr_type recv_buf() { return m_recv_buf; }

            virtual buf_ptr_type send_buf() { return m_send_buf; }

            std::shared_ptr<message> front_message() 
            { 
                //std::unique_lock<std::mutex> lock(m_mutex); 
                return m_queue.size() ? m_queue.front() : nullptr; 
            }

            const channel_source & channel_source() const { return m_channel_id; }

            void set_state(channel_state state) { m_state = state; }

            channel_state get_state() const { return m_state; }

            std::string addr_info() const { return m_addr_info; }

            void init_addr_info()
            {
                m_addr_info = " local: " + m_socket.local_endpoint().address().to_string() + " " + std::to_string(m_socket.local_endpoint().port()) + " "
                                        + "remote: " + m_socket.remote_endpoint().address().to_string() + " " + std::to_string(m_socket.remote_endpoint().port());
            }

            virtual int32_t init()
            {
                LOG_DEBUG << "tcp channel init: " << m_str_channel_id;

                init_opt();

                init_buf();

                m_queue.clear();

                return ERR_SUCCESS;
            }

            virtual int32_t close()
            {
                m_state = CHANNEL_INACTIVE;
                LOG_DEBUG << "tcp channel close: " << m_str_channel_id << m_addr_info;

                boost::system::error_code error;

                m_socket.cancel(error);
                if (error)
                {
                    LOG_ERROR << "tcp channel cancel error: " << error << m_str_channel_id;
                }

                m_socket.close(error);
                if (error)
                {
                    LOG_ERROR << "tcp channel close error: " << error << m_str_channel_id;
                }

                m_queue.clear();

                m_recv_buf->reset();
                m_send_buf->reset();

                return ERR_SUCCESS;
            }

            virtual int32_t exit()
            {
                close();

                return ERR_SUCCESS;
            }

            virtual int32_t read()
            {
                if (CHANNEL_INACTIVE == m_state)
                {
                    return ERR_FAILED;
                }

                assert(nullptr != m_recv_buf);

                m_recv_buf->move_buf();

                if (0 == m_recv_buf->get_valid_write_len())
                {
                    std::runtime_error e("tcp channel recv buffer full");
                    m_inbound_chain.fire_exception_caught(e);

                    return ERR_FAILED;
                }

                assert(nullptr != shared_from_this());

                //handler chain
                m_inbound_chain.fire_channel_read();

                m_socket.async_read_some(boost::asio::buffer(m_recv_buf->get_write_ptr(), m_recv_buf->get_valid_write_len()),
                    boost::bind(&tcp_channel::on_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

                return ERR_SUCCESS;
            }

            virtual int32_t write(std::shared_ptr<message> msg)
            {
                if (CHANNEL_INACTIVE == m_state)
                {
                    LOG_ERROR << "tcp channel write error: channel INACTIVE" << this->m_channel_id.to_string() << addr_info();
                    return ERR_FAILED;
                }

                try
                {
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);

                        if (m_queue.size() >= MAX_QUEUE_SIZE)
                        {
                            LOG_ERROR << "tcp channel send queue is full: " << msg->get_name() << " " << m_str_channel_id;
                            return ERR_FAILED;
                        }

                        if (m_queue.size())
                        {
                            LOG_DEBUG << "tcp channel send msg: " << msg->get_name() << " " << m_str_channel_id;
                            m_queue.push_back(msg);
                            return ERR_SUCCESS;
                        }

                        m_queue.push_back(msg);
                    }

                    if (m_send_buf->get_valid_read_len() > 0)
                    {
                        m_send_buf->reset();
                    }

                    assert(nullptr != shared_from_this());

                    if (CHANNEL_INACTIVE == m_state)    //for safety and efficiency
                    {
                        LOG_ERROR << "tcp channel write error: channel INACTIVE" << this->m_channel_id.to_string() << addr_info();
                        return ERR_FAILED;
                    }

                    //handler chain
                    m_outbound_chain.fire_channel_write();

                    LOG_DEBUG << m_str_channel_id << " send buf: " << m_send_buf->to_string();

                    m_socket.async_write_some(boost::asio::buffer(m_send_buf->get_read_ptr(), m_send_buf->get_valid_read_len()),
                        boost::bind(&tcp_channel::on_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR << "tcp channel write std exception: " << e.what() << m_str_channel_id << addr_info();
                    m_outbound_chain.fire_exception_caught(e);
                }
                catch (const boost::exception & e)
                {
                    std::runtime_error err("tcp channel write boost exception: " + m_str_channel_id + boost::diagnostic_information(e));
                    LOG_ERROR << "tcp channel write boost exception: " << boost::diagnostic_information(e) << m_str_channel_id << addr_info();

                    m_outbound_chain.fire_exception_caught(err);
                }
                catch (...)
                {
                    LOG_ERROR << "tcp channel write exception" << m_str_channel_id << addr_info();

                    std::runtime_error e("tcp channel write exception: " + m_str_channel_id);
                    m_outbound_chain.fire_exception_caught(e);
                }

                return ERR_SUCCESS;
            }

        protected:

            void init_opt()
            {
                m_recv_buf_len = m_opts.count("MAX_RECV_BUF_LEN") ? boost::any_cast<uint32_t>(m_opts.get("MAX_RECV_BUF_LEN")) : MAX_RECV_BUF_LEN;
                m_send_buf_len = m_opts.count("MAX_SEND_BUF_LEN") ? boost::any_cast<uint32_t>(m_opts.get("MAX_SEND_BUF_LEN")) : MAX_SEND_BUF_LEN;
                assert(m_recv_buf_len <= MAX_RECV_BUF_LEN && m_send_buf_len <= MAX_SEND_BUF_LEN);
            }

            void init_buf()
            {
                m_recv_buf = std::make_shared<io_streambuf>(m_recv_buf_len);
                m_send_buf = std::make_shared<io_streambuf>(m_send_buf_len);
            }

            void on_read(const boost::system::error_code& error, size_t bytes_transferred)
            {
                if (CHANNEL_INACTIVE == m_state)
                {
                    LOG_ERROR << "tcp channel is INACTIVE on read and exit directly" << m_str_channel_id << addr_info();
                    return;
                }

                if (error)
                {
                    if (TCP_CHANNEL_TRANSFER_TIMEOUT == error.value())
                    {
                        LOG_ERROR << "tcp channel on read timeout: " << error.value() << " " << error.message() << m_str_channel_id << addr_info();
                        read();
                        return;
                    }

                    m_state = CHANNEL_INACTIVE;

                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        LOG_ERROR << "tcp channel on read aborted: " << error.value() << " " << error.message() << m_str_channel_id << addr_info();
                        return;
                    }

                    LOG_ERROR << "tcp channel on read error: " << error.value() << " " << error.message() << m_str_channel_id << addr_info();

                    //handler chain
                    m_inbound_chain.fire_exception_caught(std::runtime_error("tcp channel on read error"));
                    return;
                }

                if (0 == bytes_transferred)
                {
                    read();
                    return;
                }

                try
                {
                    if (ERR_SUCCESS != m_recv_buf->move_write_ptr((std::uint32_t)bytes_transferred))
                    {
                        LOG_ERROR << "tcp channel on read move write ptr error" << m_str_channel_id << addr_info();
                        m_inbound_chain.fire_exception_caught(std::runtime_error("tcp channel on read move write ptr error"));
                        return;
                    }

                    LOG_DEBUG << m_str_channel_id << " recv buf: " << m_recv_buf->to_string() << addr_info();

                    //handler chain
                    m_inbound_chain.fire_channel_read_complete();

                    read();
                }
                catch (const std::exception & e)
                {
                    LOG_ERROR << "tcp channel on read std exception: " << e.what() << m_str_channel_id << addr_info();
                    m_inbound_chain.fire_exception_caught(e);
                }
                catch (const boost::exception & e)
                {
                    std::runtime_error err("tcp channel on read boost exception: " + m_str_channel_id + boost::diagnostic_information(e));
                    LOG_ERROR << "tcp channel on read boost exception" << err.what() << m_str_channel_id << addr_info();

                    m_inbound_chain.fire_exception_caught(err);
                }
                catch (...)
                {
                    LOG_ERROR << "tcp channel on read exception" << m_str_channel_id << addr_info();
                    std::runtime_error e("tcp channel on read exception: " + m_str_channel_id);

                    m_inbound_chain.fire_exception_caught(e);
                }

            }

            void on_write(const boost::system::error_code& error, size_t bytes_transferred)
            {
                if (CHANNEL_INACTIVE == m_state)
                {
                    return;
                }

                if (error)
                {
                    m_state = CHANNEL_INACTIVE;

                    //aborted, maybe cancel triggered
                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        LOG_ERROR << "tcp channel on write aborted: " << error.value() << " " << error.message() << m_str_channel_id << addr_info();
                        return;
                    }

                    LOG_ERROR << "tcp channel on write aborted: " << error.value() << " " << error.message() << m_str_channel_id << addr_info();

                    std::runtime_error e("tcp connector on write error: " + error.message());
                    m_outbound_chain.fire_exception_caught(e);
                    return;
                }

                try
                {
                    if (0 == bytes_transferred)
                    {
                        do_write(m_send_buf);
                    }
                    else if (bytes_transferred < m_send_buf->get_valid_read_len())
                    {
                        m_send_buf->move_read_ptr((uint32_t)bytes_transferred);
                        do_write(m_send_buf);
                    }
                    else if (bytes_transferred == m_send_buf->get_valid_read_len())
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);

                        m_send_buf->reset();

                        std::shared_ptr<message> msg = m_queue.front();

                        //handler chain
                        m_outbound_chain.fire_channel_write_complete();

                        msg = nullptr;

                        m_queue.pop_front();

                        //send next message
                        if (0 != m_queue.size())
                        {
                            //handler chain
                            m_outbound_chain.fire_channel_write();

                            do_write(m_send_buf);
                        }
                    }
                    else
                    {
                        std::runtime_error e("tcp channel on write error: bytes_transferred larger than valid len");
                        LOG_ERROR << "tcp channel on write error: bytes_transferred larger than valid len" << m_str_channel_id << addr_info();
                        m_outbound_chain.fire_exception_caught(e);
                    }
                }
                catch (const std::exception & e)
                {
                    LOG_ERROR << "tcp channel on write std exception: " << e.what() << m_str_channel_id << addr_info();
                    m_outbound_chain.fire_exception_caught(e);
                }
                catch (const boost::exception & e)
                {
                    std::runtime_error err("tcp channel on write boost exception: " + m_str_channel_id + boost::diagnostic_information(e));
                    LOG_ERROR << "tcp channel on write boost exception: " << boost::diagnostic_information(e) << m_str_channel_id << addr_info();

                    m_outbound_chain.fire_exception_caught(err);
                }
                catch (...)
                {
                    LOG_ERROR << "tcp channel on write exception" << m_str_channel_id << addr_info();

                    std::runtime_error e("tcp channel on write exception: " + m_str_channel_id);
                    m_outbound_chain.fire_exception_caught(e);
                }
            }

            void do_write(std::shared_ptr<io_streambuf> &msg_buf)
            {
                if (CHANNEL_INACTIVE == m_state)
                {
                    return;
                }

                m_socket.async_write_some(boost::asio::buffer(msg_buf->get_read_ptr(), msg_buf->get_valid_read_len()),
                    boost::bind(&tcp_channel::on_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            }

        protected:

            channel_type_id m_channel_id;

            std::string m_str_channel_id;

            any_map m_opts;

            chain_type m_inbound_chain;

            chain_type m_outbound_chain;

            initializer_ptr_type m_inbound_initializer;

            initializer_ptr_type m_outbound_initializer;

            channel_state m_state;

            socket_type m_socket;

            ios_ptr_type m_ios;

            buf_ptr_type m_recv_buf;

            uint32_t m_recv_buf_len;

            buf_ptr_type m_send_buf;

            uint32_t m_send_buf_len;

            queue_type m_queue;

            mutex_type m_mutex;

            endpoint_type m_remote_addr;

            endpoint_type m_local_addr;

            std::string m_addr_info;

        };

    }

}