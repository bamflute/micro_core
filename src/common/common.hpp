#pragma once

#include <time.h>
#include <sys/timeb.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef __cplusplus
#define __BEGIN_DECLS__               extern "C" {
#define __END_DECLS__                     }
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

    static inline uint64_t get_mill_seconds_from_19700101()
    {
        struct timeb  tm;
        ftime(&tm);
        return tm.time * 1000 + tm.millitm;
    }

    static inline uint64_t get_seconds_from_19700101()
    {
        return time(NULL);
    }

    static inline uint64_t get_microseconds_of_day()
    {
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::hours(8);
        return now.time_of_day().total_microseconds();
    }

};

