#include <test_udp.h>
#include <io/udp_channel.hpp>
#include <memory>
#include <thread>


using namespace micro::core;
using namespace std::chrono_literals;


class echo_udp_body : public base_body
{
public:

    std::string m_echo;
};

class echo_udp_client_handler : public channel_inbound_handler, public channel_outbound_handler
{
public:

    void channel_read_complete(context_type &ctx)
    {
        static int client_count = 0;

        auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
        assert(nullptr != ch);

        boost::asio::ip::udp::endpoint remote_endpoint = ch->get_remote_endpoint();

        std::shared_ptr<io_streambuf> recv_buf = ch->recv_buf();
        assert(nullptr != recv_buf);

        if (0 == recv_buf->get_valid_read_len())
        {
            return ctx.fire_channel_read_complete();
        }

        std::string str(recv_buf->get_read_ptr(), recv_buf->get_valid_read_len());
        std::cout << str << std::endl;

        std::shared_ptr<message> msg = std::make_shared<message>();
        std::shared_ptr<echo_udp_body> msg_body = std::make_shared<echo_udp_body>();
        msg_body->m_echo = "hello world, I'm client: " + std::to_string(++client_count);
        msg->m_body = msg_body;
        msg->m_header->m_dst.m_endpoint = remote_endpoint;

        //std::cout << ">> " << msg_body->m_echo << std::endl;

        //buf->move_read_ptr(buf->get_valid_read_len());
        //recv_buf->reset();

        ch->write(msg);
    }

    void channel_write(context_type &ctx)
    {
        auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
        assert(nullptr != ch);

        //get front message
        std::shared_ptr<message> msg = ch->front_message();
        assert(nullptr != msg);

        //pop up front message
        ch->pop_front_message();

        std::shared_ptr<echo_udp_body> msg_body = std::dynamic_pointer_cast<echo_udp_body>(msg->m_body);
        if (0 == msg_body->m_echo.size())
        {
            return ctx.fire_channel_write();
        }

        send_data *snd_data = new send_data();                        //malloc should check

        uv_ip4_addr(GET_IP(msg->get_dst_endpoint()), msg->get_dst_endpoint().port(), &snd_data->m_send_addr);

        snd_data->m_uv_buf = (uv_buf_t *)malloc(sizeof (uv_buf_t));
        snd_data->m_uv_buf->base = (char *)malloc(msg_body->m_echo.size());
        snd_data->m_uv_buf->len = (ULONG)msg_body->m_echo.size();
        memcpy(snd_data->m_uv_buf->base, msg_body->m_echo.c_str(), snd_data->m_uv_buf->len);

        ch->get_send_bufs().push_back(snd_data);

        return ctx.fire_channel_write();
    }
};

class echo_udp_client_initializer : public io_handler_initializer
{
public:

    void init(context_chain & chain)
    {
        chain.add_last("echo handler", std::make_shared<echo_udp_client_handler>());
    }
};

class echo_udp_server_handler : public channel_inbound_handler, public channel_outbound_handler
{
public:

    void channel_read_complete(context_type &ctx)
    {
        static int server_count = 0;
        auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
        assert(nullptr != ch);

        boost::asio::ip::udp::endpoint remote_endpoint = ch->get_remote_endpoint();

        std::shared_ptr<io_streambuf> recv_buf = ch->recv_buf();
        assert(nullptr != recv_buf);

        if (0 == recv_buf->get_valid_read_len())
        {
            return ctx.fire_channel_read_complete();
        }

        std::string str(recv_buf->get_read_ptr(), recv_buf->get_valid_read_len());
        std::cout << str << std::endl;

        std::shared_ptr<message> msg = std::make_shared<message>();
        std::shared_ptr<echo_udp_body> msg_body = std::make_shared<echo_udp_body>();
        msg_body->m_echo = "hello world, I'm server: " + std::to_string(++server_count);
        msg->m_body = msg_body;
        msg->m_header->m_dst.m_endpoint = remote_endpoint;

        //std::cout << ">> " << msg_body->m_echo << std::endl;

        //buf->move_read_ptr(buf->get_valid_read_len());
        //recv_buf->reset();

        ch->write(msg);
    }

    void channel_write(context_type &ctx)
    {
        auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
        assert(nullptr != ch);

        //get front message
        std::shared_ptr<message> msg = ch->front_message();
        assert(nullptr != msg);

        //pop up front message
        ch->pop_front_message();

        std::shared_ptr<echo_udp_body> msg_body = std::dynamic_pointer_cast<echo_udp_body>(msg->m_body);
        if (0 == msg_body->m_echo.size())
        {
            return ctx.fire_channel_write();
        }

        send_data *snd_data = new send_data();                        //malloc should check

        uv_ip4_addr(GET_IP(msg->get_dst_endpoint()), msg->get_dst_endpoint().port(), &snd_data->m_send_addr);

        snd_data->m_uv_buf = (uv_buf_t *)malloc(sizeof(uv_buf_t));
        snd_data->m_uv_buf->base = (char *)malloc(msg_body->m_echo.size());
        snd_data->m_uv_buf->len = (ULONG)msg_body->m_echo.size();
        memcpy(snd_data->m_uv_buf->base, msg_body->m_echo.c_str(), snd_data->m_uv_buf->len);

        ch->get_send_bufs().push_back(snd_data);

        return ctx.fire_channel_write();
    }
};

class echo_udp_server_initializer : public io_handler_initializer
{
public:

    void init(context_chain & chain)
    {
        chain.add_last("echo handler", std::make_shared<echo_udp_server_handler>());
    }
};

int test_udp(int argc, char* argv[])
{
    std::shared_ptr<uv_thread_pool> pool;
    pool = std::make_shared<uv_thread_pool>();
    pool->init();

    pool->start();

    //udp server
    boost::asio::ip::udp::endpoint server_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 39999);
    //boost::asio::ip::udp::endpoint server_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 39999);
    //boost::asio::ip::udp::endpoint server_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 9999);
    std::shared_ptr<udp_channel> server_channel = std::make_shared<udp_channel>(pool.get(), server_endpoint);

    server_channel->channel_initializer(std::make_shared<echo_udp_server_initializer>(), std::make_shared<echo_udp_server_initializer>());
    server_channel->init();

    auto client_channel = std::make_shared<udp_channel>(pool.get(), udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 19999));
    client_channel->channel_initializer(std::make_shared<echo_udp_client_initializer>(), std::make_shared<echo_udp_client_initializer>());
    client_channel->init();

    //pool->start();

    std::shared_ptr<message> msg = std::make_shared<message>();
    std::shared_ptr<echo_udp_body> msg_body = std::make_shared<echo_udp_body>();
    //msg_body->m_echo = std::to_string(i);
    //msg_body->m_echo.resize(1200);
    msg->m_body = msg_body;

    //msg->set_name(std::to_string(i));

    msg->m_header->m_dst.m_endpoint = server_endpoint;

    //uint64_t begin_timestamp = time_util::get_microseconds_of_day();
    //uint64_t cur_timestamp = begin_timestamp;

    for (int i = 0; i < 1; i++)
    {
        //msg->set_name(std::to_string(i));
        msg_body->m_echo = "let's begin";
        client_channel->write(msg);
    }

    //cur_timestamp = time_util::get_microseconds_of_day();
    //LOG_DEBUG << "test udp cost: " << std::to_string(cur_timestamp - begin_timestamp) << " us";

    //pool->start();

    while (true)
    {
        //std::cout << "Hello, Bruce" << std::endl;
        std::this_thread::sleep_for(2s);
    }

    return 0;
}