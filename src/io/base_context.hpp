#pragma once


#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <common/any_map.hpp>


namespace micro
{
    namespace core
    {

        class base_context
        {
        public:

            typedef boost::asio::ip::tcp::endpoint endpoint_type;

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

            virtual void fire_exception_caught(const std::exception & e) = 0;

            virtual void fire_accepted() = 0;

            virtual void fire_connected() = 0;

            virtual void fire_channel_active() = 0;

            virtual void fire_channel_inactive() = 0;

            virtual void fire_channel_read() = 0;

            virtual void fire_channel_read_complete() = 0;

            virtual void fire_channel_write() = 0;

            virtual void fire_channel_write_complete() = 0;

            virtual void fire_channel_batch_write() = 0;

            virtual void fire_channel_batch_write_complete() = 0;

            virtual void fire_bind(const endpoint_type &local_addr) = 0;

            virtual void fire_connect(const endpoint_type &remote_addr) = 0;

            virtual void fire_close() = 0;

        protected:

            any_map m_vars;
        };

    }

}