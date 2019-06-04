#pragma once


#include <memory>
#include <io/io_handler.hpp>
#include <io/io_context.hpp>


#define IO_CONTEXT    "io_context"


namespace micro
{
    namespace core
    {

        class context_chain
        {
        public:

            typedef std::shared_ptr<io_handler> handler_ptr_type;
            typedef std::shared_ptr<io_context> context_ptr_type;

            context_chain() : m_head(std::make_shared<head_context>()) {}

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
            
            virtual context_chain & add_last(std::string name, handler_ptr_type handler)
            {
                context_ptr_type ctx = new_context(name, handler);

                //set connector/channel/acceptor
                assert(m_vars.count(IO_CONTEXT));
                ctx->set(IO_CONTEXT, m_vars.get(IO_CONTEXT));

                assert(ctx != nullptr);

                if (nullptr == m_head)
                {
                    m_head->m_next = ctx;
                    ctx->m_prev = m_head;
                    return  *this;
                }

                assert(nullptr != m_head);

                context_ptr_type tail = get_tail();
                tail->m_next = ctx;
                ctx->m_prev = tail;

                return *this;
            }

            virtual void fire_exception_caught(const std::exception & e)
            {
                if (m_head) m_head->fire_exception_caught(e);
            }

            virtual void fir_accepted()
            {
                if (m_head) m_head->fire_accepted();
            }   

            virtual void fire_connected()
            {
                if (m_head) m_head->fire_connected();
            }
            
            virtual void fire_channel_active()
            {
                if (m_head) m_head->fire_channel_active();
            }

            virtual void fire_channel_inactive()
            {
                if (m_head) m_head->fire_channel_inactive();
            }

            virtual void fire_channel_read()
            {
                if (m_head) m_head->fire_channel_read();
            }

            virtual void fire_channel_read_complete()
            {
                if (m_head) m_head->fire_channel_read_complete();
            }

            virtual void fire_channel_write()
            {
                if (m_head) m_head->fire_channel_write();
            }

            virtual void fire_channel_write_complete()
            {
                if (m_head) m_head->fire_channel_write_complete();
            }

            virtual void fire_channel_writablity_changed()
            {
                if (m_head) m_head->fire_channel_writablity_changed();
            }

            virtual void bind(const socket_address &local_addr)
            {
                if (m_head) m_head->fire_channel_writablity_changed();
            }         

            virtual void connect(const socket_address &local_addr, const socket_address &remote_addr)
            {
                if (m_head) m_head->fire_connect(local_addr, remote_addr);
            }

            virtual void close()
            {
                if (m_head) m_head->close();
            }


        protected:

            context_ptr_type new_context(std::string name, handler_ptr_type handler)
            {
                return std::make_shared<io_context>(name, handler);
            }

            context_ptr_type get_tail()
            {
                context_ptr_type ctx = m_head;
                context_ptr_type tail = ctx;

                while (ctx->m_next)
                {
                    ctx = ctx->m_next;
                    tail = ctx;
                }

                return tail;
            }

        protected:

            any_map m_vars;

            context_ptr_type m_head;

        };

    }

}