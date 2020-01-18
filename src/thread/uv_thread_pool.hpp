#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <thread>
#include <uv.h>

#include <common/error.hpp>

#define DEFAULT_UV_THREAD_COUNT         1

#define BOOTSTRAP_UV_POOL(POOL, IOS_SIZE) \
    POOL = std::make_shared<uv_thread_pool>(); \
    POOL->init(IOS_SIZE); \
    POOL->start();

extern void uv_thrad_func(void *arg);

namespace micro
{
    namespace core
    {

        class uv_thread_pool
        {
        public:

            typedef std::function<void(void *)> functor_type;

            typedef std::shared_ptr<std::thread> thr_ptr_type;

            uv_thread_pool()
                : m_loop(nullptr) 
                , m_exited(false)
                , m_functor(uv_thrad_func)
            {
            }

            ~uv_thread_pool() = default;

            int32_t init(size_t size = DEFAULT_UV_THREAD_COUNT)
            {
                m_loop = uv_loop_new();

                return nullptr != m_loop ? ERR_SUCCESS : ERR_FAILED;
            }

            int32_t start()
            {
                m_thr = std::make_shared<std::thread>(m_functor, this);
                return ERR_SUCCESS;
            }

            int32_t stop()
            {
                m_exited = true;
                uv_stop(m_loop);
                return ERR_SUCCESS;
            }

            int32_t exit()
            {
                uv_loop_delete(m_loop);

                return ERR_SUCCESS;
            }

            uv_loop_t * get_loop()
            {
                return m_loop;
            }

            int32_t run()
            {
                while (!m_exited)
                {
                    uv_run(m_loop, UV_RUN_DEFAULT);
                }

                return ERR_SUCCESS;
            }

        protected:

            bool m_exited;

            thr_ptr_type m_thr;

            uv_loop_t * m_loop;

            functor_type m_functor;

        };

    }

}