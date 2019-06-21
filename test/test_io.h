#pragma once


#include <io/tcp_connector.hpp>
#include <io/tcp_acceptor.hpp>
#include <io/base_context.hpp>
#include <io/io_streambuf.hpp>
#include <io/channel.hpp>
#include <io/channel_id_allocator.h>
#include <io/context_chain.hpp>
#include <io/head_context.hpp>
#include <io/io_handler.hpp>
#include <io/io_handler_initializer.hpp>
#include <io/tcp_channel.hpp>
#include <message/message.hpp>


using namespace micro::core;

extern "C" int test_io(int argc, char* argv[]);


class echo_body : public base_body
{
public:

    std::string m_echo;
};

class echo_connector_handler : public tcp_connector_handler
{
public:

    void connected(context_type &ctx) 
    {
        auto connector = boost::any_cast<std::shared_ptr<tcp_connector>>(ctx.get(std::string(IO_CONTEXT)));

        std::shared_ptr<tcp_channel> ch = connector->channel();
        assert(nullptr != ch);

        std::shared_ptr<message> msg = std::make_shared<message>();
        std::shared_ptr<echo_body> msg_body = std::make_shared<echo_body>();
        msg_body->m_echo = "hello world, I'm client.";
        msg->m_body = msg_body;

        ch->write(msg);

        ctx.fire_connected(); 
    }
};

class echo_handler : public channel_inbound_handler, public channel_outbound_handler
{
public:

    void channel_read_complete(context_type &ctx)
    {
        auto ch = boost::any_cast<std::shared_ptr<tcp_channel>>(ctx.get(std::string(IO_CONTEXT)));
        assert(nullptr != ch);

        std::shared_ptr<io_streambuf> buf = ch->recv_buf();
        assert(nullptr != buf);

        if (0 == buf->get_valid_read_len())
        {
            return ctx.fire_channel_read_complete();
        }
        
        std::shared_ptr<message> msg = std::make_shared<message>();
        std::shared_ptr<echo_body> msg_body = std::make_shared<echo_body>();

        if (ch->channel_source().m_channel_type == SERVER_TYPE)
        {
            msg_body->m_echo = "hello world, I'm server.";
        }
        else
        {
            msg_body->m_echo = "hello world, I'm client.";
        }
        //msg_body->m_echo.assign(buf->get_read_ptr(), buf->get_valid_read_len());
        msg->m_body = msg_body;

        std::cout << ">> " << msg_body->m_echo << std::endl;

        buf->move_read_ptr(buf->get_valid_read_len());

        ch->write(msg);
    }

    void channel_write(context_type &ctx)
    {
        auto ch = boost::any_cast<std::shared_ptr<tcp_channel>>(ctx.get(std::string(IO_CONTEXT)));
        assert(nullptr != ch);

        std::shared_ptr<message> msg = ch->front_message();
        assert(nullptr != msg);

        std::shared_ptr<echo_body> msg_body = std::dynamic_pointer_cast<echo_body>(msg->m_body);
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

class echo_connector_initializer : public io_handler_initializer
{
public:

    void init(context_chain & chain)
    {
        chain.add_last("echo connector handler", std::make_shared<echo_connector_handler>());
    }
};

class echo_initializer : public io_handler_initializer
{
public:

    void init(context_chain & chain) 
    { 
        chain.add_last("echo handler", std::make_shared<echo_handler>());
    }
};

