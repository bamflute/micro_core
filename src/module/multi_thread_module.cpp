#include "multi_thread_module.hpp"


void worker_task_func(void *arg)
{
    micro::core::worker_thread *worker = (micro::core::worker_thread *)arg;
    worker->run();
}

