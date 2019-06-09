#pragma once


#include <io/channel.hpp>

namespace micro
{
    namespace core
    {

        class io_handler_initializer
        {
        public:

            virtual ~io_handler_initializer() = default;

            virtual void init(context_chain & chain) = 0;

        };

    }

}