#include <io/udp_channel.hpp>


using namespace micro::core;

//callback function
void on_read_callback(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf, const struct sockaddr* addr, unsigned flags)
{
    BEGIN_TIME_COST("on read callback cost time: ")
    udp_channel * ch = LIB_UV_GET_CHANNEL_POINTER(handle);
    ch->on_read(nread, addr);
    END_TIME_COST
}

void on_write_callback(uv_udp_send_t* req, int status)
{
    BEGIN_TIME_COST("on write callback  cost time: ")

    //callback udp channel
    udp_channel * ch = LIB_UV_GET_CHANNEL_POINTER(req->handle);

    //get uv send data
    send_data *snd_data = (send_data *)uv_handle_get_data((uv_handle_t*)req);

    if (snd_data->m_extra_info && (0 == ((uint64_t)(snd_data->m_extra_info)) % 10000))
    {
        LOG_INFO << "file extra info: " << std::to_string((uint64_t)(snd_data->m_extra_info));
    }

    //assert
    assert(snd_data->m_uv_buf_count > 0 && snd_data->m_uv_buf != nullptr);

    ch->on_write(status);
    
    //send buffer base
    for (int i = 0; i < snd_data->m_uv_buf_count; i++)
    {
        free((snd_data->m_uv_buf + i)->base);
    }

    free(snd_data->m_uv_buf);
    free(snd_data);
    free(req);

    END_TIME_COST
}

void on_close_callback(uv_handle_t* handle)
{
    uv_is_closing(handle);
}

void on_async_callback(uv_async_t* handle)
{
    udp_channel * ch = (udp_channel *)uv_handle_get_data((uv_handle_t*)handle);
    if (ch)
    {
        ch->on_async();
    }
}
