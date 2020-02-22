#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <message/message.hpp>
#include <io/io_streambuf.hpp>
#include <io/io_handler_initializer.hpp>
#include <io/context_chain.hpp>
#include <thread/nio_thread_pool.hpp>
#include <io/io_macro.hpp>
#include <thread/uv_thread_pool.hpp>


__BEGIN_DECLS__
#include <uv.h>
__END_DECLS__


#define MAX_UDP_RECV_BUF_LEN       10240
#define MAX_UDP_SEND_BUF_LEN       10240
#define MAX_UDP_QUEUE_SIZE             102400


#define LIB_UV_GET_CHANNEL_POINTER(HANDLE)              (*((udp_channel**)((char *)HANDLE - 8)))           //64 bit OS

using boost::asio::ip::udp;

__BEGIN_DECLS__
extern void on_read_callback(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf, const struct sockaddr* addr, unsigned flags);
extern void on_write_callback(uv_udp_send_t* req, int status);
extern void on_close_callback(uv_handle_t* handle);
extern void on_async_callback(uv_async_t* handle);
__END_DECLS__


namespace micro
{

    namespace core
    {

        class send_data
        {
        public:

            uv_buf_t * m_uv_buf = nullptr;

            size_t m_uv_buf_count = 0;

            sockaddr_in m_send_addr;
        };

        class batch_send_message
        {
        public:

            std::list<std::shared_ptr<message>> m_msgs;

            boost::asio::ip::udp::endpoint m_dst_endpoint;
        };

        class udp_channel : public std::enable_shared_from_this<udp_channel>
        {
        public:

            typedef std::mutex mutex_type;
            typedef context_chain chain_type;
            typedef std::shared_ptr<io_streambuf> buf_ptr_type;
            typedef boost::asio::ip::udp::endpoint endpoint_type;
            typedef std::list<std::shared_ptr<message>> msg_queue_type;
            typedef std::list<send_data *> send_buf_queue_type;
            typedef std::shared_ptr<micro::core::nio_thread_pool> thr_pool_ptr_type;
            typedef std::shared_ptr<micro::core::io_handler_initializer> initializer_ptr_type;
            typedef std::shared_ptr<boost::asio::io_service> ios_ptr_type;
            typedef std::list < std::shared_ptr<batch_send_message>> batch_msg_queue_type;


            udp_channel(std::shared_ptr<uv_thread_pool> pool, endpoint_type endpoint)
                : m_pool(pool)
                , m_local_endpoint(endpoint)
                , m_recv_buf(std::make_shared<io_streambuf>())
            {
                m_self = this;
                udp_channel * ch = LIB_UV_GET_CHANNEL_POINTER(&m_socket);
            }

            //handler chain
            context_chain & inbound_pipeline() { return m_inbound_chain; }

            context_chain & outbound_pipeline() { return m_outbound_chain; }

            buf_ptr_type recv_buf() { return m_recv_buf; }

            boost::asio::ip::udp::endpoint get_remote_endpoint() { return m_remote_endpoint; }

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

            //single message queue
            msg_queue_type & get_msg_queue() { return m_msg_queue; }

            std::shared_ptr<message> front_message() { return m_msg_queue.size() ? m_msg_queue.front() : nullptr; }

            void pop_front_message() { m_msg_queue.pop_front(); }

            //batch send message
            batch_msg_queue_type & get_batch_msg_queue() { return m_batch_msg_queue; }

            std::shared_ptr<batch_send_message> front_batch_message() { return m_batch_msg_queue.size() ? m_batch_msg_queue.front() : nullptr; }

            void pop_front_batch_message() { m_batch_msg_queue.pop_front(); }

            //send bufs
            send_buf_queue_type & get_send_bufs() { return m_send_bufs; }

            virtual int32_t init()
            {
                //loop init and async callback
                uv_udp_init(m_pool->get_loop(), &m_socket);

                uv_async_init(m_pool->get_loop(), &m_async, on_async_callback);
                uv_handle_set_data((uv_handle_t*)&m_async, (void*)this);

                //bind addr
                uv_ip4_addr(GET_IP(m_local_endpoint), m_local_endpoint.port(), &m_addr);
                uv_udp_bind(&m_socket, (const struct sockaddr*) &m_addr, UV_UDP_REUSEADDR);

                return this->read(); 
            }

            virtual int32_t close()
            {
                //close
                uv_close((uv_handle_t*)&m_socket, on_close_callback);

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

                {
                    std::unique_lock<std::mutex> lock(m_mutex);

                    m_msg_queue.push_back(msg);                                     //pending ready to encode message

                    //handle chain
                    m_outbound_chain.fire_channel_write();
                }

                //async notify
                int r = uv_async_send(&m_async);
                if (0 != r)
                {
                    LOG_ERROR << "udp channeluv async send error: " << std::to_string(r);
                }

                return ERR_SUCCESS;
            }

            //warning: all msg dst endpoint should be same
            virtual int32_t batch_write(std::shared_ptr<batch_send_message> msg)
            {
                {
                    std::unique_lock<std::mutex> lock(m_mutex);

                    m_batch_msg_queue.push_back(msg);                       //batch msg to encode to send buffer

                    //handle chain
                    m_outbound_chain.fire_channel_batch_write();
                }

                //async notify
                int r = uv_async_send(&m_async);
                if (0 != r)
                {
                    LOG_ERROR << "udp channeluv async send error: " << std::to_string(r);
                }

                return ERR_SUCCESS;
            }

            void on_async()
            {
                try
                {
                    do_write();
                }
                catch (const std::exception & e)
                {
                    LOG_ERROR << "udp channel on async std exception: " << e.what();
                    m_outbound_chain.fire_exception_caught(e);
                }
                catch (...)
                {
                    LOG_ERROR << "udp channel on async exception";

                    std::runtime_error e("udp channel on write exception");
                    m_outbound_chain.fire_exception_caught(e);
                }
            }

            void on_write(int status)
            {
                if (status)
                {
                    LOG_ERROR << "udp channel on write error: " << status;

                    //handler chain
                    m_outbound_chain.fire_exception_caught(std::runtime_error("udp channel on write error"));
                }

                do_write();
            }

            static void alloc_recv_io_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
            {
                udp_channel * ch = LIB_UV_GET_CHANNEL_POINTER(handle);
                buf->base = ch->recv_buf()->get_write_ptr();
                buf->len = ch->recv_buf()->get_valid_write_len();
            }

            virtual int32_t read()
            {
                int sock_buf_len = (int)(m_recv_buf->get_valid_read_len());
                uv_recv_buffer_size((uv_handle_t*)&m_socket, &sock_buf_len);

                uv_udp_recv_start(&m_socket, alloc_recv_io_buf, on_read_callback);

                return ERR_SUCCESS;
            }

            void on_read(ssize_t bytes_transferred, const struct sockaddr* addr)
            {
                if (bytes_transferred < 0)
                {
                    LOG_ERROR << "udp channel on read error";

                    //handler chain
                    m_inbound_chain.fire_exception_caught(std::runtime_error("udp channel on read error"));
                    return;
                }

                if (0 == bytes_transferred)
                {
                    //LOG_ERROR << "udp channel on read bytes ZERO";
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

                    struct sockaddr_in *sock = (struct sockaddr_in*)addr;
                    char *saddr = inet_ntoa(sock->sin_addr);
                    int port = ntohs(sock->sin_port);
                    endpoint_type remote_endpoint(boost::asio::ip::address::from_string(saddr), port);
                    m_remote_endpoint = remote_endpoint;

                    //LOG_DEBUG << "remote ip: " << m_remote_endpoint.address().to_string() << " port: " << std::to_string(m_remote_endpoint.port());

                    //handler chain
                    m_inbound_chain.fire_channel_read_complete();

                    m_recv_buf->reset();
                }
                catch (const std::exception & e)
                {
                    LOG_ERROR << "udp channel on read std exception: " << e.what();
                    m_inbound_chain.fire_exception_caught(e);
                }
                catch (...)
                {
                    LOG_ERROR << "udp channel on read exception";
                    std::runtime_error e("udp channel on read exception");

                    m_inbound_chain.fire_exception_caught(e);
                }
            }

        protected:

            void do_write()
            {

                try
                {
                    std::unique_lock<std::mutex> lock(m_mutex);

                    while (!m_send_bufs.empty())
                    {
                        send_data *snd_data = m_send_bufs.front();

                        //register data
                        uv_udp_send_t *send_req = (uv_udp_send_t *)malloc(sizeof(uv_udp_send_t));
                        uv_handle_set_data((uv_handle_t*)send_req, (void*)snd_data);

                        assert(snd_data->m_uv_buf_count > 0 && snd_data->m_uv_buf != nullptr);

                        //uv send
                        int r = uv_udp_send(send_req, &m_socket, snd_data->m_uv_buf, (unsigned int)snd_data->m_uv_buf_count, (const struct sockaddr*) &snd_data->m_send_addr, on_write_callback);
                        if (r)
                        {
                            LOG_ERROR << "uv_udp_send error: " << std::to_string(r);
                        }

                        m_send_bufs.pop_front();
                    }

                }
                catch (const std::exception & e)
                {
                    LOG_ERROR << "udp channel on write std exception: " << e.what();
                    m_outbound_chain.fire_exception_caught(e);
                }
                catch (...)
                {
                    LOG_ERROR << "udp channel on write exception";

                    std::runtime_error e("udp channel on write exception");
                    m_outbound_chain.fire_exception_caught(e);
                }
            }

        protected:

            udp_channel * m_self;               //tricky define, have to define this for libuv C style get channel pointer; m_self must be close to m_socket

            uv_udp_t m_socket;

            uv_async_t m_async;

            struct sockaddr_in m_addr;

            std::shared_ptr<uv_thread_pool> m_pool;

            endpoint_type m_local_endpoint;

            msg_queue_type m_msg_queue;

            batch_msg_queue_type m_batch_msg_queue;

            send_buf_queue_type m_send_bufs;

            mutex_type m_mutex;

            buf_ptr_type m_recv_buf;

            endpoint_type m_remote_endpoint;

            chain_type m_inbound_chain;

            chain_type m_outbound_chain;

            initializer_ptr_type m_inbound_initializer;

            initializer_ptr_type m_outbound_initializer;

        };

    }

}
