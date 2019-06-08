#pragma once


#include <io/byte_buf.hpp>
#include <io/socket_address.hpp>


namespace micro
{
    namespace core
    {

        class base_context
        {
        public:

            typedef boost::asio::ip::tcp::endpoint endpoint_type;

            virtual void fire_exception_caught(const std::exception & e) = 0;

            virtual void fire_accepted() = 0;

            virtual void fire_connected() = 0;

            virtual void fire_channel_active() = 0;

            virtual void fire_channel_inactive() = 0;

            virtual void fire_channel_read() = 0;

            virtual void fire_channel_read_complete() = 0;

            virtual void fire_channel_write() = 0;

            virtual void fire_channel_write_complete() = 0;

            virtual void fire_bind(const endpoint_type &local_addr) = 0;

            virtual void fire_connect(const endpoint_type &remote_addr) = 0;

            virtual void fire_close() = 0;

        };

    }

}