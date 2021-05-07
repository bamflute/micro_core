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
    
    static std::string time_2_str(time_t t)
    {
        struct tm _tm;
#ifdef WIN32
        int err = 0;
        err = localtime_s(&_tm, &t);
        if (err != 0)
        {
            //std::cout << "get local time err: " << err << std::endl;
            return "";
        }
#else
        localtime_r(&t, &_tm);
#endif
        char buf[256];
        memset(buf, 0, sizeof(char) * 256);
        strftime(buf, sizeof(char) * 256, "%x %X", &_tm);

        return buf;
    }
    
    static std::string time_2_str_date(time_t t)
    {
        struct tm _tm;
#ifdef WIN32
        int err = 0;
        err = localtime_s(&_tm, &t);
        if (err != 0)
        {
            //std::cout << "get local time err: " << err << std::endl;
            return "";
        }
#else
        localtime_r(&t, &_tm);
#endif
        char buf[256];
        memset(buf, 0, sizeof(char) * 256);
        strftime(buf, sizeof(char) * 256, "%G%m%d", &_tm);

        return buf;
    }
    
    static std::string time_2_str_time(time_t t)
    {
        struct tm _tm;
#ifdef WIN32
        int err = 0;
        err = localtime_s(&_tm, &t);
        if (err != 0)
        {
            //std::cout << "get local time err: " << err << std::endl;
            return "";
        }
#else
        localtime_r(&t, &_tm);
#endif
        char buf[256];
        memset(buf, 0, sizeof(char) * 256);
        strftime(buf, sizeof(char) * 256, "%G%m%d%H%M%S", &_tm);
        return buf;
    }

    static std::string time_2_utc(time_t t)
    {
        std::string temp = std::asctime(std::gmtime(&t));
        
        return temp.erase(temp.find("\n"));
    }
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
