#pragma once


#include <io/io_context.hpp>


namespace micro
{
    namespace core
    {

        class head_context : public io_context
        {
        public:

            head_context() : io_context("head_context", nullptr) {} 

        };

    }

}