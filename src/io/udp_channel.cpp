#include <io/udp_channel.hpp>


using namespace micro::core;

//callback function
void on_read_callback(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf, const struct sockaddr* addr, unsigned flags)
{
    //uint64_t timestamp = time_util::get_microseconds_of_day();
    //LOG_DEBUG << "====================read timestamp: " << std::to_string(timestamp) << " ====================";

    udp_channel * ch = LIB_UV_GET_CHANNEL_POINTER(handle);
    ch->on_read(nread, addr);
}

void on_write_callback(uv_udp_send_t* req, int status)
{
    //callback udp channel
    udp_channel * ch = LIB_UV_GET_CHANNEL_POINTER(req->handle);
    ch->on_write(status);

    //get uv buf data
    uv_buf_t * buf = (uv_buf_t *)uv_handle_get_data((uv_handle_t*)req);

    free(buf->base);
    free(buf);
    free(req);
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
