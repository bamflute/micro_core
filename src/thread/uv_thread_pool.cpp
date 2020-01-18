#include <thread/uv_thread_pool.hpp>

void uv_thrad_func(void *arg)
{
    micro::core::uv_thread_pool *pool = (micro::core::uv_thread_pool *)arg;
    pool->run();
}