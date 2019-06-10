#pragma once


#include <memory>
#include <boost/asio.hpp>
#include <io/tcp_channel.hpp>
#include <io/io_handler_initializer.hpp>
#include <thread/nio_thread_pool.hpp>
#include <common/error.hpp>
#include <io/channel_id_allocator.h>


#define ACCEPTOR_LISTEN_BACKLOG 64

namespace micro
{
    namespace core
    {

        class tcp_acceptor : public std::enable_shared_from_this<tcp_acceptor>, boost::noncopyable
        {
        public:

            typedef context_chain chain_type;
            typedef std::shared_ptr<tcp_channel> channel_ptr_type;
            typedef boost::asio::ip::tcp::endpoint endpoint_type;
            typedef std::weak_ptr<boost::asio::io_service> ios_weak_ptr_type;
            typedef std::shared_ptr<nio_thread_pool> thr_pool_ptr_type;
            typedef std::shared_ptr<io_handler_initializer> initializer_ptr_type;
            typedef channel_source channel_type_id;
            typedef boost::asio::ip::tcp::acceptor acceptor_type;

            tcp_acceptor(ios_weak_ptr_type ios, endpoint_type endpoint)
                : m_ios(ios)
                , m_endpoint(endpoint)
                , m_acceptor(*ios.lock(), endpoint, true)
            {
                
            }

            virtual ~tcp_acceptor() = default;


            context_chain & context_chain() { return m_context_chain; }

            template<typename T>
            void acceptor_option(std::string name, T value)
            {
                m_acceptor_opts.set(name, value);
            }

            template<typename T>
            void channel_option(std::string name, T value)
            {
                m_channel_opts.set(name, value);
            }

            void group(thr_pool_ptr_type acceptor_thr_pool, thr_pool_ptr_type channel_thr_pool)
            {
                m_acceptor_thr_pool = acceptor_thr_pool;
                m_channel_thr_pool = channel_thr_pool;
            }

            void acceptor_initializer(initializer_ptr_type acceptor_initializer)
            {
                if (nullptr == acceptor_initializer)
                {
                    return;
                }

                m_context_chain.set(IO_CONTEXT, this->shared_from_this());

                m_acceptor_initializer = acceptor_initializer;
                m_acceptor_initializer->init(m_context_chain);
            }

            void channel_inbound_initializer(initializer_ptr_type channel_inbound_initializer)
            {
                assert(nullptr != channel_inbound_initializer);
                m_channel_inbound_initializer = channel_inbound_initializer;
            }

            void channel_outbound_initializer(initializer_ptr_type channel_outbound_initializer)
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


            virtual int32_t init()
            {
                boost::system::error_code error;
                m_acceptor.listen(ACCEPTOR_LISTEN_BACKLOG, error);

                if (error)
                {
                    return ERR_FAILED;
                }

                return accept();
            }

            virtual int32_t exit()
            {
                boost::system::error_code error;

                m_acceptor.cancel(error);
                if (error)
                {
                    LOG_ERROR << "tcp acceptor cancel error: " << error.message();
                }

                m_acceptor.close(error);
                if (error)
                {
                    LOG_ERROR << "tcp acceptor close error: " << error.message();
                }

                return ERR_SUCCESS;
            }

        protected:

            virtual int32_t accept()
            {
                channel_type_id channel_id(SERVER_TYPE, get_new_channel_id());
                auto ch = std::make_shared<tcp_channel>(m_channel_thr_pool->get_ios(), channel_id);
                assert(nullptr != ch);

                //channel option

                //channel handler initializer
                ch->channel_inbound_initializer(m_channel_inbound_initializer);
                ch->channel_outbound_initializer(m_channel_outbound_initializer);

                assert(nullptr != ch->shared_from_this());
                m_acceptor.async_accept(ch->socket(), boost::bind(&tcp_acceptor::on_accept, shared_from_this(), ch->shared_from_this(), boost::asio::placeholders::error));

                return ERR_SUCCESS;
            }

            virtual int32_t on_accept(std::shared_ptr<tcp_channel> ch, const boost::system::error_code& error)
            {
                if (nullptr == ch)
                {
                    return ERR_FAILED;
                }

                if (error)
                {
                    //aborted, maybe cancel triggered
                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        return ERR_FAILED;
                    }

                    std::runtime_error err("tcp acceptor error: " + error.value());
                    m_context_chain.fire_exception_caught(err);

                    accept();

                    return ERR_FAILED;
                }

                try
                {
                    //handler chain
                    m_context_chain.fire_accepted();

                    ch->init();
                    ch->read();
                }
                catch (const boost::exception & e)
                {
                    std::runtime_error err("tcp acceptor error: " + boost::diagnostic_information(e));
                    m_context_chain.fire_exception_caught(err);

                    accept();
                    return ERR_FAILED;
                }

                return accept();

            }

        protected:

            any_map m_acceptor_opts;

            any_map m_channel_opts;

            endpoint_type m_endpoint;

            acceptor_type m_acceptor;

            ios_weak_ptr_type m_ios;

            thr_pool_ptr_type m_acceptor_thr_pool;

            thr_pool_ptr_type m_channel_thr_pool;

            chain_type m_context_chain;

            initializer_ptr_type m_channel_inbound_initializer;

            initializer_ptr_type m_channel_outbound_initializer;

            initializer_ptr_type m_acceptor_initializer;

        };

    }

}