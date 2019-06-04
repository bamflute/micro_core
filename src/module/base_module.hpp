/*********************************************************************************
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
*  date: 2017.10.01
*  author:  Bruce Feng
**********************************************************************************/
#pragma once

#include <string>
#include <boost/program_options.hpp>
#include <memory>
#include <timer/base_timer.hpp>
#include <common/error.hpp>



namespace micro
{
    namespace core
    {

        class base_module
        {
        public:

            typedef boost::program_options::variables_map var_type;

            virtual ~base_module() {}

            virtual std::string name() const { return "base module"; };

            virtual int32_t init(var_type &vars) { return ERR_SUCCESS; }

            virtual int32_t exit() { return ERR_SUCCESS; }

            virtual int32_t start() { return ERR_SUCCESS; }

            virtual int32_t stop() { return ERR_SUCCESS; }

            virtual int32_t on_timer(std::shared_ptr<base_timer> timer) { return ERR_SUCCESS; }

        };

    }

}
