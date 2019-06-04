#pragma once


#include <boost/asio.hpp>
#include <io/context_chain.hpp>

namespace micro
{
    namespace core
    {

        enum channel_state
        {
            CHANNEL_ACTIVE,
            CHANNEL_STOPPED
        };

        class channel
        {
        public:

            virtual context_chain & pipeline() = 0;

            virtual boost::asio::ip::tcp::socket & socket() = 0;

            virtual int32_t start() = 0;

            virtual int32_t stop() = 0;

            virtual int32_t read() = 0;

            virtual int32_t write(std::shared_ptr<message> msg) = 0;

        };

    }

}