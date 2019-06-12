#pragma once

#include <functional>


typedef std::function<void(uint64_t)>  functor_type;


#define INVALID_TIMER_ID                                                   0
#define MAX_TIMER_ID                                                          0xFFFFFFFFFFFFFFFF
#define DEFAULT_MILLISECONDS_ONE_TICK                      100                                                  //100 ms for one tick


