#include <module/module_func.h>
#include <module/module.hpp>


void timer_func(void *arg, std::shared_ptr<micro::core::timer> timer)
{
    assert(arg);
    micro::core::module *mdl = (micro::core::module *)arg;
    mdl->on_time_out(timer);
}
