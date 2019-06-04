#pragma once

#include <memory>
#include <timer/timer.hpp>


extern "C" void timer_func(void *arg, std::shared_ptr<micro::core::timer> timer);

