#include "channel_id_allocator.h"
#include <common/common.hpp>

automic_uint64_type g_channel_id(0);


std::uint64_t get_new_channel_id()
{
    ++g_channel_id;
    return g_channel_id.load();
}