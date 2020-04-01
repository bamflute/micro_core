#include "svc_thread.hpp"

void svc_thread_func(void *arg)
{
    assert(arg);
    micro::core::svc_thread *thr = (micro::core::svc_thread *)arg;
    thr->run();
}