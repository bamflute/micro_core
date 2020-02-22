#pragma once


#include <common/any_map.hpp>
#include <common/common.hpp>
#include <io/mask.hpp>
#include <io/base_context.hpp>
#include <io/io_handler.hpp>


namespace micro
{
    namespace core
    {
        class io_context : public base_context
        {
        public:

            typedef std::shared_ptr<io_handler> handler_ptr_type;
            typedef std::shared_ptr<io_context> context_ptr_type;

            io_context(std::string name, handler_ptr_type handler)
                : m_name(name)
                , m_handler(handler)
                , m_next(nullptr)
                , m_prev(nullptr)
                , m_mask(m_handler ? m_handler->mask() : 0)
            {}

            virtual ~io_context() { clear(); }

            void clear()
            {
                m_vars.clear();
                
                m_next = nullptr;
                m_prev = nullptr;
                m_handler = nullptr;
            }                

            handler_ptr_type handler() { return m_handler; }         

            virtual void fire_exception_caught(const std::exception & e)
            {
                context_ptr_type next = find_next_context(MASK_EXCEPTION_CAUGHT);
                if (next)
                {
                    next->invoke_exception_caught(e);
                }
            }

            virtual void invoke_exception_caught(const std::exception & e)
            {
                if (m_handler)
                {
                    m_handler->exception_caught(*this, e);
                }
            }
            
            virtual void fire_accepted()
            {
                context_ptr_type next = find_next_context(MASK_ACCEPTED);
                if (next)
                {
                    next->invoke_accepted();
                }
            }

            virtual void invoke_accepted()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(tcp_acceptor_handler, m_handler);
                    assert(handler != nullptr);
                    handler->accepted(*this);
                }
            }

            virtual void fire_connected()
            {
                context_ptr_type next = find_next_context(MASK_CONNECTED);
                if (next)
                {
                    next->invoke_connected();
                }
            }

            virtual void invoke_connected()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(tcp_connector_handler, m_handler);
                    assert(handler != nullptr);
                    handler->connected(*this);
                }
            }

            virtual void fire_channel_active()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_ACTIVE);
                if (next)
                {
                    next->invoke_channel_active();
                }
            }

            virtual void invoke_channel_active()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_inbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_active(*this);
                }
            }

            virtual void fire_channel_inactive()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_INACTIVE);
                if (next)
                {
                    next->invoke_channel_inactive();
                }
            }

            virtual void invoke_channel_inactive()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_inbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_inactive(*this);
                }
            }

            virtual void fire_channel_read()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_READ);
                if (next)
                {
                    next->invoke_channel_read();
                }
            }

            virtual void invoke_channel_read()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_inbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_read(*this);
                }
            }

            virtual void fire_channel_read_complete()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_READ_COMPLETE);
                if (next)
                {
                    next->invoke_channel_read_complete();
                }
            }

            virtual void invoke_channel_read_complete()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_inbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_read_complete(*this);
                }
            }

            virtual void fire_channel_write()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_WRITE);
                if (next)
                {
                    next->invoke_channel_write();
                }
            }

            virtual void invoke_channel_write()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_outbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_write(*this);
                }
            }

            virtual void fire_channel_write_complete()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_WRITE_COMPLETE);
                if (next)
                {
                    next->invoke_channel_write_complete();
                }
            }

            virtual void invoke_channel_write_complete()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_outbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_write_complete(*this);
                }
            }

            virtual void fire_channel_batch_write()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_BATCH_WRITE);
                if (next)
                {
                    next->invoke_channel_batch_write();
                }
            }

            virtual void invoke_channel_batch_write()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_outbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_batch_write(*this);
                }
            }

            virtual void fire_channel_batch_write_complete()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_BATCH_WRITE_COMPLETE);
                if (next)
                {
                    next->invoke_channel_batch_write_complete();
                }
            }

            virtual void invoke_channel_batch_write_complete()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_outbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_batch_write_complete(*this);
                }
            }

            virtual void fire_bind(const endpoint_type &local_addr)
            {
                context_ptr_type next = find_next_context(MASK_BIND);
                if (next)
                {
                    next->invoke_bind(local_addr);
                }
            }

            virtual void invoke_bind(const endpoint_type &local_addr)
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(tcp_connector_handler, m_handler);
                    assert(handler != nullptr);
                    handler->bind(*this, local_addr);
                }
            }

            virtual void fire_connect(const endpoint_type &remote_addr)
            {
                context_ptr_type next = find_next_context(MASK_CONNECT);
                if (next)
                {
                    next->invoke_connect(remote_addr);
                }
            }

            virtual void invoke_connect(const endpoint_type &remote_addr)
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(tcp_connector_handler, m_handler);
                    assert(handler != nullptr);
                    handler->connect(*this, remote_addr);
                }
            }

            virtual void fire_close()
            {
                context_ptr_type next = find_next_context(MASK_CLOSE);
                if (next)
                {
                    next->invoke_close();
                }
            }

            virtual void invoke_close()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_outbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->close(*this);
                }
            }

        protected:

            context_ptr_type find_next_context(uint32_t mask)
            {
                context_ptr_type ctx = this->m_next;
                while ((ctx != nullptr) && (0 == (ctx->m_mask & mask)))
                {
                    ctx = ctx->m_next;
                }
                return ctx;
            }

        public:

            context_ptr_type m_next;

            context_ptr_type m_prev;

        protected:

            std::string m_name;

            handler_ptr_type m_handler;

            std::uint32_t m_mask;

        };

    }

}