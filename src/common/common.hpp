#pragma once

#include <atomic>
#include <cstdint>
#include <boost/program_options.hpp>

#ifdef __cplusplus
#define __BEGIN_DECLS__               extern "C" {
#define __END_DECLS__                                        }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#define DYN_CAST(T, OBJ)    std::dynamic_pointer_cast<T>(OBJ)

#ifdef WIN32
typedef std::atomic_uint64_t automic_uint64_type;
#else
typedef std::atomic<std::uint64_t> automic_uint64_type;
#endif

typedef boost::program_options::variables_map var_type;


class time_util
{
public:

    static uint64_t get_seconds_from_19700101();

    static uint64_t get_mill_seconds_from_19700101();

    static uint64_t get_micro_seconds_from_19700101();

    static uint64_t get_microseconds_of_day();

};

extern bool g_enable_time_cost;

extern void enable_time_cost();

extern void disable_time_cost();

class time_cost_util
{
public:

    time_cost_util(const char * tips);

    virtual ~time_cost_util();

protected:

    const char * m_tips;

    uint64_t m_begin_timestamp;

};

#define BEGIN_TIME_COST(TIPS) \
{\
time_cost_util tcu(TIPS);

#define END_TIME_COST \
}