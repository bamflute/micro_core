#pragma once


#include <io/channel.hpp>

namespace micro
{
    namespace core
    {

        class channel_initializer
        {
        public:

            virtual void init(context_chain & chain) = 0;

        };

    }

}