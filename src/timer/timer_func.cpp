#include <timer/timer_func.h>
#include <module/module.hpp>

void task_func(void *arg)
{
    micro::core::module *mdl = (micro::core::module *)arg;
    mdl->run();
}