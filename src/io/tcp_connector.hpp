#pragma once



#include <boost/asio.hpp>
#include <common/option.hpp>
#include <thread/nio_thread_pool.hpp>
#include <io/error.hpp>
#include <io/channel.hpp>
#include <io/io_handler.hpp>
#include <io/channel_context_pipeline.hpp>
#include <io/tcp_channel.hpp>
#include <io/channel_initializer.hpp>
#include <io/channel_id_allocator.h>


#define MAX_RECONNECT_TIMES         0xFFFFFFFF


namespace micro
{
    namespace core
    {

        class tcp_connector : public std::enable_shared_from_this<tcp_connector>, public boost::noncopyable
        {
        public:

            typedef std::shared_ptr<nio_thread_pool> thr_pool_ptr_type;
            typedef boost::asio::ip::tcp::endpoint endpoint_type;
            typedef std::shared_ptr<io_handler> handler_ptr_type;
            typedef std::shared_ptr<channel> channel_ptr_type;
            typedef context_chain chain_type;
            typedef std::shared_ptr<channel_initializer> channel_initializer_ptr_type;

            tcp_connector() : m_connected(false) {}

            virtual ~tcp_connector() = default;

            bool is_connected() { return m_connected; }

            context_chain & context_chain() { return m_context_chain; }

            template<typename T>
            void connector_option(std::string name, T value)
            {
                m_connector_opts.option(name, value);
            }

            template<typename T>
            void channel_option(std::string name, T value)
            {
                m_channel_opts.option(name, value);
            }

            void group(thr_pool_ptr_type connector_thr_pool, thr_pool_ptr_type channel_thr_pool)
            {
                m_connector_thr_pool = connector_thr_pool;
                m_channel_thr_pool = channel_thr_pool;
            }

            void connector_initializer(channel_initializer_ptr_type connector_initializer)
            {
                m_connector_initializer = connector_initializer;

                if (m_connector_initializer)
                {
                    m_connector_initializer->init(m_context_chain);
                }
            }

            void channel_initializer(channel_initializer_ptr_type channel_initializer)
            {
                m_channel_initializer = channel_initializer;
            }

            virtual int32_t connect(const endpoint_type &remote_addr)
            {
                if (nullptr == m_connector_thr_pool || nullptr == m_channel_thr_pool)
                {
                    return ERR_FAILED;
                }

                m_channel->socket().async_connect(remote_addr, boost::bind(&tcp_connector::on_connect, shared_from_this(), boost::asio::placeholders::error));
            }

            virtual void on_connect(const boost::system::error_code& error)
            {
                if (error)
                {
                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        LOG_DEBUG << "tcp_connector on connect aborted.";
                        return;
                    }

                    std::runtime_error e("tcp connect error");
                    m_context_chain.fire_exception_caught(e);

                    return;
                }

                try
                {
                    m_channel = std::make_shared<tcp_channel>(m_channel_thr_pool, channel_source(CLIENT_TYPE, get_new_channel_id()));

                    //option

                    //channel initializer
                    m_channel->channel_initializer(m_channel_initializer);

                    m_channel->start();
                    m_channel->read();
                }
                catch (const boost::exception & e)
                {
                    std::string errorinfo = diagnostic_information(e);
                    return;
                }

                m_connected = true;
            }

            virtual int32_t stop()
            {
                if (true == m_connected)
                {
                    return ERR_SUCCESS;
                }

                boost::system::error_code error;

                std::dynamic_pointer_cast<tcp_channel>(m_channel)->socket().cancel(error);
                if (error)
                {
                    LOG_ERROR << "tcp connector cancel error: " << error;
                }

                std::dynamic_pointer_cast<tcp_channel>(m_channel)->socket().close(error);
                if (error)
                {
                    LOG_ERROR << "tcp connector close error: " << error;
                }

                return ERR_SUCCESS;
            }

        protected:

            bool m_connected;

            any_map m_connector_opts;

            any_map m_channel_opts;

            channel_ptr_type m_channel;            

            thr_pool_ptr_type m_channel_thr_pool;

            thr_pool_ptr_type m_connector_thr_pool;

            chain_type m_context_chain;

            channel_initializer_ptr_type m_channel_initializer;

            channel_initializer_ptr_type m_connector_initializer;
        };

    }

}