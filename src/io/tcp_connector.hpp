#pragma once



#include <boost/asio.hpp>
#include <common/any_map.hpp>
#include <thread/nio_thread_pool.hpp>
#include <common/error.hpp>
#include <io/channel.hpp>
#include <io/io_handler.hpp>
#include <io/context_chain.hpp>
#include <io/tcp_channel.hpp>
#include <io/io_handler_initializer.hpp>
#include <io/channel_id_allocator.h>


#define MAX_RECONNECT_TIMES         0xFFFFFFFF


namespace micro
{
    namespace core
    {

        class tcp_connector : public std::enable_shared_from_this<tcp_connector>, public boost::noncopyable
        {
        public:

            typedef context_chain chain_type;
            typedef std::shared_ptr<io_handler> handler_ptr_type;            
            typedef std::shared_ptr<tcp_channel> channel_ptr_type;
            typedef boost::asio::ip::tcp::endpoint endpoint_type;
            typedef std::shared_ptr<nio_thread_pool> thr_pool_ptr_type;
            typedef std::shared_ptr<io_handler_initializer> initializer_ptr_type;
            typedef channel_source channel_type_id;

            tcp_connector() : m_connected(false) { }

            virtual ~tcp_connector() { m_context_chain.clear(); }

            bool is_connected() { return m_connected; }            

            context_chain & context_chain() { return m_context_chain; }

            channel_ptr_type channel() { return m_channel; }

            const channel_source & channel_source() { return this->channel()->channel_source(); }


            template<typename T>
            void connector_option(std::string name, T value)
            {
                m_connector_opts.set(name, value);
            }

            template<typename T>
            void channel_option(std::string name, T value)
            {
                m_channel_opts.set(name, value);
            }

            void group(thr_pool_ptr_type connector_thr_pool, thr_pool_ptr_type channel_thr_pool)
            {
                m_connector_thr_pool = connector_thr_pool;
                m_channel_thr_pool = channel_thr_pool;
            }

            void connector_initializer(initializer_ptr_type connector_initializer)
            {
                if (nullptr == connector_initializer)
                {
                    return;
                }

                m_context_chain.set(IO_CONTEXT, this->shared_from_this());
            
                m_connector_initializer = connector_initializer;
                m_connector_initializer->init(m_context_chain);
            }

            void channel_inbound_initializer(initializer_ptr_type channel_inbound_initializer)
            {
                assert(nullptr != channel_inbound_initializer);
                m_channel_inbound_initializer = channel_inbound_initializer;
            }

            void channel_outound_initializer(initializer_ptr_type channel_outbound_initializer)
            {
                assert(nullptr != channel_outbound_initializer);
                m_channel_outbound_initializer = channel_outbound_initializer;
            }

            void channel_initializer(initializer_ptr_type channel_inbound_initializer, initializer_ptr_type channel_outbound_initializer)
            {
                assert(nullptr != channel_inbound_initializer && nullptr != channel_outbound_initializer);

                m_channel_inbound_initializer = channel_inbound_initializer;
                m_channel_outbound_initializer = channel_outbound_initializer;
            }

            int32_t init()
            {
                //channel
                channel_type_id channel_id(CLIENT_TYPE, get_new_channel_id());
                m_channel = std::make_shared<tcp_channel>(m_channel_thr_pool->get_ios(), channel_id);

                //channel option

                //channel handler initializer
                m_channel->channel_inbound_initializer(m_channel_inbound_initializer);
                m_channel->channel_outbound_initializer(m_channel_outbound_initializer);

                m_channel->init();
                return ERR_SUCCESS;
            }

            virtual int32_t connect(const endpoint_type &remote_addr)
            {
                this->m_remote_addr = remote_addr;
                return connect();
            }

            virtual int32_t connect()
            {
                if (nullptr == m_connector_thr_pool || nullptr == m_channel_thr_pool)
                {
                    return ERR_FAILED;
                }

                //handler chain
                m_context_chain.fire_connect(m_remote_addr);

                m_channel->socket().async_connect(m_remote_addr, boost::bind(&tcp_connector::on_connect, shared_from_this(), boost::asio::placeholders::error));

                return ERR_SUCCESS;
            }

            virtual void on_connect(const boost::system::error_code& error)
            {
                if (error)
                {
                    m_connected = false;
                    m_channel->set_state(CHANNEL_INACTIVE);

                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        LOG_ERROR << "tcp_connector connect aborted " << m_channel->channel_source().to_string() << m_channel->addr_info();
                        return;
                    }

                    LOG_ERROR << "tcp_connector connect error: " << error.value() << m_channel->channel_source().to_string() << m_channel->addr_info();

                    //handler chain
                    std::runtime_error e("tcp connector error: " + error.value());
                    m_context_chain.fire_exception_caught(e);

                    return;
                }

                m_connected = true;

                m_channel->set_state(CHANNEL_ACTIVE);
                m_channel->init_addr_info();

                //handler chain
                m_context_chain.fire_connected();

                try
                {
                    assert(nullptr != m_channel);
                    m_channel->read();
                }
                catch (const boost::exception & e)
                {
                    std::string errorinfo = boost::diagnostic_information(e);
                    return;
                }

            }

            virtual int32_t close()
            {
                m_channel->close();

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

            initializer_ptr_type m_channel_inbound_initializer;

            initializer_ptr_type m_channel_outbound_initializer;

            initializer_ptr_type m_connector_initializer;

            endpoint_type m_remote_addr;
            
        };

    }

}