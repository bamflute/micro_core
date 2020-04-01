#include "multi_thread_module.hpp"

thread_local uint32_t thread_local_idx = 0;

void worker_task_func(void *arg)
{
    micro::core::worker_thread *worker = (micro::core::worker_thread *)arg;
    worker->run();
}

