#pragma once


#include <boost/asio.hpp>
#include <io/context_chain.hpp>
#include <message/message.hpp>

namespace micro
{
    namespace core
    {

        enum channel_state
        {
            CHANNEL_ACTIVE,
            CHANNEL_INACTIVE
        };

        class channel
        {
        public:

            virtual boost::asio::ip::tcp::socket & socket() = 0;

            virtual int32_t init() = 0;

            virtual int32_t exit() = 0;

            virtual int32_t read() = 0;

            virtual int32_t write(std::shared_ptr<micro::core::message> msg) = 0;

        };

    }

}