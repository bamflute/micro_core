#pragma once

#include <string>
#include <memory>
#include <timer/base_timer.hpp>
#include <common/error.hpp>
#include <common/common.hpp>
#include <common/any_map.hpp>



namespace micro
{
    namespace core
    {

        class base_module
        {
        public:

            virtual ~base_module() {}

            virtual std::string name() const { return "base module"; };

            virtual int32_t init(any_map &vars) { return ERR_SUCCESS; }

            virtual int32_t exit() { return ERR_SUCCESS; }

            virtual int32_t start() { return ERR_SUCCESS; }

            virtual int32_t stop() { return ERR_SUCCESS; }

            virtual int32_t on_timer(std::shared_ptr<base_timer> timer) { return ERR_SUCCESS; }

        };

    }

}
