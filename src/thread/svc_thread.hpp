#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <thread>
#include <common/any_map.hpp>
#include <common/common.hpp>
#include <common/error.hpp>


extern void svc_thread_func(void *arg);


namespace micro
{
    namespace core
    {
        class svc_thread
        {
        public:

            typedef std::function<void(void *)> functor_type;

            typedef std::shared_ptr<std::thread> thr_ptr_type;

            svc_thread()
                : m_exited(false)
                , m_functor(svc_thread_func)
            {
            }

            ~svc_thread() = default;

            virtual int32_t init(any_map &vars)
            {
                return service_init(vars);
            }

            virtual int32_t start()
            {
                m_thr = std::make_shared<std::thread>(m_functor, this);
                return ERR_SUCCESS;
            }

            virtual int32_t stop()
            {
                m_exited = true;

                if (m_thr)
                {
                    try
                    {
                        m_thr->join();
                    }
                    catch (...)
                    {
                        return ERR_FAILED;
                    }
                }

                return ERR_SUCCESS;
            }

            virtual int32_t exit() { return service_exit(); }

            virtual int32_t run()
            {
                while (!m_exited)
                {
                    service_run();
                }

                return ERR_SUCCESS;
            }

        protected:

            virtual int32_t service_init(any_map &vars) { return ERR_SUCCESS; }

            virtual int32_t service_exit() { return ERR_SUCCESS; }

            virtual int32_t service_run() { return ERR_SUCCESS; }

        protected:

            bool m_exited;

            thr_ptr_type m_thr;

            functor_type m_functor;

        };

    }

}