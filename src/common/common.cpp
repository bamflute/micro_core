#include <common/common.hpp>
#include <time.h>
#include <sys/timeb.h>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <logger/logger.hpp>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

uint64_t time_util::get_seconds_from_19700101()
{
    return time(NULL);
}

uint64_t time_util::get_mill_seconds_from_19700101()
{
    struct timeb  tm;
    ftime(&tm);
    return tm.time * 1000 + tm.millitm;
}

uint64_t time_util::get_microseconds_of_day()
{
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::hours(8);
    return now.time_of_day().total_microseconds();
}

uint64_t time_util::get_micro_seconds_from_19700101()
{

#ifdef _WIN32
    //time from 1601.01.01 0:0:0:000 to 1970.01.01 0:0:0:000 (unit: 100ms)
#define EPOCHFILETIME   (116444736000000000UL)
    FILETIME ft;
    LARGE_INTEGER li;
    int64_t tt = 0;
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    //microsecons from 1970.01.01 0:0:0:000 up to now
    tt = (li.QuadPart - EPOCHFILETIME) / 10;
    return tt;
#else
    timeval tv;
    gettimeofday(&tv, 0);
    return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
#endif
}

bool g_enable_time_cost = false;

void enable_time_cost()
{
    g_enable_time_cost = true;
}

void disable_time_cost()
{
    g_enable_time_cost = false;
}

time_cost_util::time_cost_util(const char * tips) : m_tips(tips)
{
    if (g_enable_time_cost)
    {
        m_begin_timestamp = time_util::get_microseconds_of_day();
    }
}

time_cost_util::~time_cost_util()
{
    if (g_enable_time_cost)
    {
        LOG_INFO << m_tips << std::to_string(time_util::get_microseconds_of_day() - m_begin_timestamp) << " us";
    }
}