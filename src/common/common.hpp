#pragma once

#define DYN_CAST(T, OBJ)    std::dynamic_pointer_cast<T>(OBJ)

#ifdef WIN32
typedef std::atomic_uint64_t automic_uint64_type;
#else
typedef std::atomic<std::uint64_t> automic_uint64_type;
#endif

