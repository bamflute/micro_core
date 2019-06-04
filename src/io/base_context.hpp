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

            virtual void fire_exception_caught(const std::exception & e) = 0;

            virtual void fire_accepted() = 0;

            virtual void fire_connected() = 0;

            virtual void fire_channel_active() = 0;

            virtual void fire_channel_inactive() = 0;

            virtual void fire_channel_read() = 0;

            virtual void fire_channel_read_complete() = 0;

            virtual void fire_channel_write() = 0;

            virtual void fire_channel_write_complete() = 0;

            virtual void fire_channel_writablity_changed() = 0;

            virtual void bind(const socket_address &local_addr) = 0;

            virtual void connect(const socket_address &local_addr, const socket_address &remote_addr) = 0;

            virtual void close() = 0;

        };

    }

}