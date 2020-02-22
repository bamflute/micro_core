#pragma once


#include <memory>
#include <io/io_handler.hpp>
#include <io/io_context.hpp>
#include <io/head_context.hpp>


#define IO_CONTEXT    "io_context"
#define REMOTE_ENDPOINT "remote_endpoint"


namespace micro
{
    namespace core
    {

        class context_chain
        {
        public:

            typedef std::shared_ptr<io_handler> handler_ptr_type;
            typedef std::shared_ptr<io_context> context_ptr_type;
            typedef boost::asio::ip::tcp::endpoint endpoint_type;

            context_chain() : m_head(std::make_shared<head_context>()) {}

            void clear()
            {
                m_vars.clear();
            }


            boost::any get(std::string var_name)
            {
                return m_vars.get(var_name);
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
                assert(nullptr != ctx);            

                assert(nullptr != m_head);
                if (nullptr == m_head->m_next)
                {
                    m_head->m_next = ctx;
                    ctx->m_prev = m_head;
                    
                    return  *this;
                }

                context_ptr_type tail = get_tail();
                tail->m_next = ctx;
                ctx->m_prev = tail;

                return *this;
            }

            virtual void fire_exception_caught(const std::exception & e)
            {
                if (m_head) m_head->fire_exception_caught(e);
            }

            virtual void fire_accepted()
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

            virtual void fire_channel_batch_write()
            {
                if (m_head) m_head->fire_channel_batch_write();
            }

            virtual void fire_channel_batch_write_complete()
            {
                if (m_head) m_head->fire_channel_batch_write_complete();
            }

            virtual void fire_bind(const endpoint_type &local_addr)
            {
                if (m_head) m_head->fire_bind(local_addr);
            }         

            virtual void fire_connect(const endpoint_type &remote_addr)
            {
                if (m_head) m_head->fire_connect(remote_addr);
            }

            virtual void fire_close()
            {
                if (m_head) m_head->fire_close();
            }


        protected:

            context_ptr_type new_context(std::string name, handler_ptr_type handler)
            {
                auto ioc = std::make_shared<io_context>(name, handler);
                
                assert(this->count(IO_CONTEXT));
                ioc->set(IO_CONTEXT, this->get(std::string(IO_CONTEXT)));

                return ioc;
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