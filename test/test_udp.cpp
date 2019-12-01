#include <test_udp.h>
#include <io/udp_channel.hpp>
#include <thread/nio_thread_pool.hpp>
#include <memory>


using namespace micro::core;


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

        std::shared_ptr<io_streambuf> buf = ch->recv_buf();
        assert(nullptr != buf);

        if (0 == buf->get_valid_read_len())
        {
            return ctx.fire_channel_read_complete();
        }

        std::string str(buf->get_read_ptr(), buf->get_valid_read_len());
        std::cout << str << std::endl;

        std::shared_ptr<message> msg = std::make_shared<message>();
        std::shared_ptr<echo_udp_body> msg_body = std::make_shared<echo_udp_body>();
        msg_body->m_echo = "hello world, I'm client: " + std::to_string(++client_count);
        msg->m_body = msg_body;
        msg->m_header->m_dst.m_endpoint = remote_endpoint;

        //std::cout << ">> " << msg_body->m_echo << std::endl;

        buf->move_read_ptr(buf->get_valid_read_len());

        ch->write(msg);
    }

    void channel_write(context_type &ctx)
    {
        auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
        assert(nullptr != ch);

        std::shared_ptr<message> msg = ch->front_message();
        assert(nullptr != msg);

        std::shared_ptr<echo_udp_body> msg_body = std::dynamic_pointer_cast<echo_udp_body>(msg->m_body);
        if (0 == msg_body->m_echo.size())
        {
            return ctx.fire_channel_write();
        }

        std::shared_ptr<io_streambuf> buf = ch->send_buf();
        assert(nullptr != buf);

        buf->write_to_byte_buf(msg_body->m_echo.c_str(), (uint32_t)msg_body->m_echo.size());

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

        std::shared_ptr<io_streambuf> buf = ch->recv_buf();
        assert(nullptr != buf);

        if (0 == buf->get_valid_read_len())
        {
            return ctx.fire_channel_read_complete();
        }

        std::string str(buf->get_read_ptr(), buf->get_valid_read_len());
        std::cout << str << std::endl;

        std::shared_ptr<message> msg = std::make_shared<message>();
        std::shared_ptr<echo_udp_body> msg_body = std::make_shared<echo_udp_body>();
        msg_body->m_echo = "hello world, I'm server: " + std::to_string(++server_count);
        msg->m_body = msg_body;
        msg->m_header->m_dst.m_endpoint = remote_endpoint;

        //std::cout << ">> " << msg_body->m_echo << std::endl;

        buf->move_read_ptr(buf->get_valid_read_len());

        ch->write(msg);
    }

    void channel_write(context_type &ctx)
    {
        auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
        assert(nullptr != ch);

        std::shared_ptr<message> msg = ch->front_message();
        assert(nullptr != msg);

        std::shared_ptr<echo_udp_body> msg_body = std::dynamic_pointer_cast<echo_udp_body>(msg->m_body);
        if (0 == msg_body->m_echo.size())
        {
            return ctx.fire_channel_write();
        }

        std::shared_ptr<io_streambuf> buf = ch->send_buf();
        assert(nullptr != buf);

        buf->write_to_byte_buf(msg_body->m_echo.c_str(), (uint32_t)msg_body->m_echo.size());

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
    BOOTSTRAP_POOL(pool, 1);

    //udp server
    boost::asio::ip::udp::endpoint server_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 9999);
    std::shared_ptr<udp_channel> server_channel = std::make_shared<udp_channel>(pool->get_ios(), server_endpoint);

    server_channel->channel_initializer(std::make_shared<echo_udp_server_initializer>(), std::make_shared<echo_udp_server_initializer>());
    server_channel->init();

    auto client_channel = std::make_shared<udp_channel>(pool->get_ios(), udp::endpoint(udp::v4(), 0));
    client_channel->channel_initializer(std::make_shared<echo_udp_client_initializer>(), std::make_shared<echo_udp_client_initializer>());
    client_channel->init();

    std::shared_ptr<message> msg = std::make_shared<message>();
    std::shared_ptr<echo_udp_body> msg_body = std::make_shared<echo_udp_body>();
    msg_body->m_echo = "hello world, I'm client";
    msg->m_body = msg_body;
    msg->m_header->m_dst.m_endpoint = server_endpoint;

    client_channel->write(msg);

    while (true)
    {

    }

    return 0;
}