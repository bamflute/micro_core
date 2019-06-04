#pragma once


#include <boost/any.hpp>
#include <common/common.hpp>
#include <io/handler_context.hpp>
#include <io/channel_inbound_handler.hpp>
#include <io/channel_outbound_handler.hpp>


namespace micro
{
    namespace core
    {
        class io_context : public base_context
        {
        public:

            typedef boost::any any_type;
            typedef std::shared_ptr<io_handler> handler_ptr_type;
            typedef std::shared_ptr<io_context> context_ptr_type;

            io_context(handler_ptr_type handler)
                : m_handler(handler)
                , m_next(nullptr)
                , m_prev(nullptr)
            {}

            template<typename T>
            T get(std::string var_name)
            {
                return m_vars.get<T>(var_name);
            }

            template<typename T>
            void set(std::string var_name, T var)
            {
                m_vars.set<T>(var_name, var);
            }

            int count(std::string name)
            {
                return m_vars.count(name);
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
                context_ptr_type next = find_next_context(MASK_CHANNEL_REGISTERED);
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
                context_ptr_type next = find_next_context(MASK_CHANNEL_REGISTERED);
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

            virtual void fire_channel_writablity_changed()
            {
                context_ptr_type next = find_next_context(MASK_CHANNEL_REGISTERED);
                if (next)
                {
                    next->invoke_channel_writablity_changed();
                }
            }

            virtual void invoke_channel_writablity_changed()
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_inbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->channel_writablity_changed(*this);
                }
            }

            virtual void bind(const socket_address &local_addr)
            {
                context_ptr_type next = find_next_context(MASK_BIND);
                if (next)
                {
                    next->invoke_bind(local_addr);
                }
            }

            virtual void invoke_bind(const socket_address &local_addr)
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_outbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->bind(*this, local_addr);
                }
            }

            virtual void connect(const socket_address &local_addr, const socket_address &remote_addr)
            {
                context_ptr_type next = find_next_context(MASK_CONNECT);
                if (next)
                {
                    next->invoke_connect(local_addr);
                }
            }

            virtual void invoke_connect(const socket_address &local_addr, const socket_address &remote_addr)
            {
                if (m_handler)
                {
                    auto handler = DYN_CAST(channel_outbound_handler, m_handler);
                    assert(handler != nullptr);
                    handler->connect(*this, local_addr, remote_addr);
                }
            }

            virtual void close()
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

            any_map m_vars;

            handler_ptr_type m_handler;

            uint32_t m_mask;

        };

        class head_context : public io_context
        {
        public:

            virtual void channel_active(context_type &ctx) { ctx.fire_channel_active(); }

            virtual void channel_inactive(context_type &ctx) { ctx.fire_channel_inactive(); }

            virtual void channel_read(context_type &ctx) { ctx.fire_channel_read(); }

            virtual void channel_read_complete(context_type &ctx) { ctx.fire_channel_read_complete(); }

            virtual void channel_writablity_changed(context_type &ctx) { ctx.fire_channel_writablity_changed(); }

            virtual void bind(context_type &ctx, const socket_address &local_addr) { ctx.bind(local_addr); }

            virtual void connect(context_type &ctx, const socket_address &local_addr, const socket_address &remote_addr) { ctx.connect(local_addr, remote_addr); }

            virtual void close(context_type &ctx) { ctx.close(); }

        };

    }

}