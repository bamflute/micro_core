#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <message/message.hpp>
#include <io/io_streambuf.hpp>
#include <io/io_handler_initializer.hpp>
#include <io/context_chain.hpp>
#include <thread/nio_thread_pool.hpp>


#define MAX_UDP_RECV_BUF_LEN       10240
#define MAX_UDP_SEND_BUF_LEN       10240
#define MAX_UDP_QUEUE_SIZE             10240


using boost::asio::ip::udp;


namespace micro
{
    namespace core
    {

        class udp_channel : public std::enable_shared_from_this<udp_channel>, public boost::noncopyable
        {
        public:

            typedef std::mutex mutex_type;
            typedef context_chain chain_type;
            typedef std::shared_ptr<io_streambuf> buf_ptr_type;
            typedef boost::asio::ip::udp::endpoint endpoint_type;
            typedef std::list<std::shared_ptr<message>> queue_type;
            typedef std::shared_ptr<micro::core::nio_thread_pool> thr_pool_ptr_type;
            typedef std::shared_ptr<micro::core::io_handler_initializer> initializer_ptr_type;
            typedef std::shared_ptr<boost::asio::io_service> ios_ptr_type;

            udp_channel(ios_ptr_type ios, endpoint_type endpoint)
                : m_ios(ios)
                , m_local_endpoint(endpoint)
                , m_socket(*ios, endpoint)
                , m_recv_buf(std::make_shared<io_streambuf>(MAX_UDP_RECV_BUF_LEN))
                , m_send_buf(std::make_shared<io_streambuf>(MAX_UDP_SEND_BUF_LEN))
            {

            }

            context_chain & inbound_pipeline() { return m_inbound_chain; }

            context_chain & outbound_pipeline() { return m_outbound_chain; }

            void channel_initializer(initializer_ptr_type channel_inbound_initializer, initializer_ptr_type channel_outbound_initializer)
            {
                assert(nullptr != channel_inbound_initializer && nullptr != channel_outbound_initializer);

                m_inbound_chain.set(IO_CONTEXT, this->shared_from_this());
                m_inbound_initializer = channel_inbound_initializer;
                m_inbound_initializer->init(m_inbound_chain);

                m_outbound_chain.set(IO_CONTEXT, this->shared_from_this());
                m_outbound_initializer = channel_outbound_initializer;
                m_outbound_initializer->init(m_outbound_chain);
            }

            virtual buf_ptr_type recv_buf() { return m_recv_buf; }

            virtual buf_ptr_type send_buf() { return m_send_buf; }

            std::shared_ptr<message> front_message() { return m_queue.size() ? m_queue.front() : nullptr; }

            const  endpoint_type & get_remote_endpoint() const { return m_remote_endpoint; }

            virtual int32_t init() { return this->read(); }

            virtual int32_t close()
            {
                boost::system::error_code error;

                m_socket.cancel(error);
                if (error)
                {
                    LOG_ERROR << "udp channel cancel error: " << error.message();
                }

                m_socket.close(error);
                if (error)
                {
                    LOG_ERROR << "udp channel close error: " << error.message();
                }

                return ERR_SUCCESS;
            }

            virtual int32_t exit()
            {
                close();

                return ERR_SUCCESS;
            }

            //////////////////////////////////////////////////////////

            virtual int32_t write(std::shared_ptr<message> msg)
            {
                try
                {
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);

                        if (m_queue.size() >= MAX_UDP_QUEUE_SIZE)
                        {
                            LOG_ERROR << "udp channel send queue is full: " << msg->get_name();
                            return ERR_FAILED;
                        }

                        if (m_queue.size())
                        {
                            LOG_DEBUG << "udp channel send msg: " << msg->get_name();
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

                    //handler chain
                    m_outbound_chain.fire_channel_write();

                    //LOG_DEBUG << "udp send buf: " << m_send_buf->to_string();

                    do_write(msg->get_dst_endpoint());
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR << "udp channel write std exception: " << e.what();
                    m_outbound_chain.fire_exception_caught(e);
                }
                catch (const boost::exception & e)
                {
                    std::runtime_error err("udp channel write boost exception: " + boost::diagnostic_information(e));
                    LOG_ERROR << "udp channel write boost exception: " << boost::diagnostic_information(e);

                    m_outbound_chain.fire_exception_caught(err);
                }
                catch (...)
                {
                    LOG_ERROR << "udp channel write exception";

                    std::runtime_error e("udp channel write exception");
                    m_outbound_chain.fire_exception_caught(e);
                }

                return ERR_SUCCESS;
            }

            virtual int32_t read()
            {
                m_socket.async_receive_from(
                    boost::asio::buffer(m_recv_buf->get_write_ptr(), m_recv_buf->get_valid_write_len()), m_remote_endpoint,
                    boost::bind(&udp_channel::on_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

                return ERR_SUCCESS;
            }

        protected:

            void do_write(endpoint_type remote_endpoint)
            {
                m_socket.async_send_to(boost::asio::buffer(m_send_buf->get_read_ptr(), m_send_buf->get_valid_read_len()), remote_endpoint,
                    boost::bind(&udp_channel::on_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            }

            void on_write(const boost::system::error_code& error, size_t bytes_transferred)
            {
                if (error)
                {
                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        LOG_ERROR << "udp channel on write aborted: " << error.value() << " " << error.message();
                        return;
                    }

                    LOG_ERROR << "udp channel on write error: " << error.value() << " " << error.message();

                    //handler chain
                    m_outbound_chain.fire_exception_caught(std::runtime_error("udp channel on read error"));
                    return;
                }

                try
                {
                    std::shared_ptr<message> msg = nullptr;

                    if (0 == bytes_transferred)
                    {
                        msg = m_queue.front();
                        do_write(msg->get_dst_endpoint());
                    }
                    else if (bytes_transferred < m_send_buf->get_valid_read_len())
                    {
                        assert(true);

                        m_send_buf->move_read_ptr((uint32_t)bytes_transferred);

                        msg = m_queue.front();
                        do_write(msg->get_dst_endpoint());
                    }
                    else if (bytes_transferred == m_send_buf->get_valid_read_len())
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);

                        m_send_buf->reset();

                        msg = m_queue.front();

                        //handler chain
                        m_outbound_chain.fire_channel_write_complete();

                        msg = nullptr;

                        m_queue.pop_front();

                        //send next message
                        if (0 != m_queue.size())
                        {
                            //handler chain
                            m_outbound_chain.fire_channel_write();

                            msg = m_queue.front();
                            do_write(msg->get_dst_endpoint());
                        }
                    }
                    else
                    {
                        std::runtime_error e("udp channel on write error: bytes_transferred larger than valid len");
                        LOG_ERROR << "udp channel on write error: bytes_transferred larger than valid len";
                        m_outbound_chain.fire_exception_caught(e);
                    }
                }
                catch (const std::exception & e)
                {
                    LOG_ERROR << "udp channel on write std exception: " << e.what();
                    m_outbound_chain.fire_exception_caught(e);
                }
                catch (const boost::exception & e)
                {
                    std::runtime_error err("udp channel on write boost exception: " + boost::diagnostic_information(e));
                    LOG_ERROR << "udp channel on write boost exception: " << boost::diagnostic_information(e);

                    m_outbound_chain.fire_exception_caught(err);
                }
                catch (...)
                {
                    LOG_ERROR << "udp channel on write exception";

                    std::runtime_error e("udp channel on write exception");
                    m_outbound_chain.fire_exception_caught(e);
                }
            }

            void on_read(const boost::system::error_code& error, size_t bytes_transferred)
            {
                if (error)
                {
                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        LOG_ERROR << "udp channel on read aborted: " << error.value() << " " << error.message();
                        return;
                    }

                    LOG_ERROR << "udp channel on read error: " << error.value() << " " << error.message();

                    //handler chain
                    m_inbound_chain.fire_exception_caught(std::runtime_error("udp channel on read error"));
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
                        LOG_ERROR << "udp channel on read move write ptr error";
                        m_inbound_chain.fire_exception_caught(std::runtime_error("udp channel on read move write ptr error"));
                        return;
                    }

                    //LOG_DEBUG << "udp recv buf: " << m_recv_buf->to_string();

                    //handler chain
                    m_inbound_chain.fire_channel_read_complete();

                    read();
                }
                catch (const std::exception & e)
                {
                    LOG_ERROR << "udp channel on read std exception: " << e.what();
                    m_inbound_chain.fire_exception_caught(e);
                }
                catch (const boost::exception & e)
                {
                    std::runtime_error err("udp channel on read boost exception: " + boost::diagnostic_information(e));
                    LOG_ERROR << "udp channel on read boost exception" << err.what();

                    m_inbound_chain.fire_exception_caught(err);
                }
                catch (...)
                {
                    LOG_ERROR << "udp channel on read exception";
                    std::runtime_error e("udp channel on read exception");

                    m_inbound_chain.fire_exception_caught(e);
                }
            }


        protected:

            ios_ptr_type m_ios;

            endpoint_type m_local_endpoint;

            endpoint_type m_remote_endpoint;

            udp::socket m_socket;

            buf_ptr_type m_recv_buf;

            buf_ptr_type m_send_buf;

            queue_type m_queue;

            mutex_type m_mutex;

            chain_type m_inbound_chain;

            chain_type m_outbound_chain;

            initializer_ptr_type m_inbound_initializer;

            initializer_ptr_type m_outbound_initializer;

        };

    }

}
