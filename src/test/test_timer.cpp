#include <test/test_timer.h>
#include <common/error.hpp>
#include <module/module.hpp>
#include <timer/timer_generator.hpp>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

class hello_world_body : public base_body
{
public:

    std::string m_hello_world;
};

class timer_module : public module
{
public:

    timer_module() = default;
    ~timer_module() = default;

    int32_t service_init(var_type &options)
    {
        INIT_INVOKER("test_msg_hello_world", &timer_module::on_msg_hello_world);
        return ERR_SUCCESS;
    }

    void init_timer()
    {
        INIT_TIMER("test_timer", 1000, MAX_TRIGGER_TIMES, "", &timer_module::on_timer_hello_world);
    }

    int32_t on_timer_hello_world(timer_ptr_type timer)
    {
        std::cout << timer->get_time_out_tick() << std::endl;
        return ERR_SUCCESS;
    }

    int32_t on_msg_hello_world(msg_ptr_type msg)
    {
        std::shared_ptr<hello_world_body> msg_body = std::dynamic_pointer_cast<hello_world_body>(msg->m_body);
        std::cout << msg_body->m_hello_world << std::endl;
        return ERR_SUCCESS;
    }

};

int test_timer(int argc, char* argv[])
{
    var_type var;
    TIMER_GENERATOR.init(var);
    TIMER_GENERATOR.start();

    auto tm = std::make_shared<timer_module>();
    tm->init(var);
    tm->start();

    while (true)
    {
        std::shared_ptr<message> msg = std::make_shared<message>();
        msg->set_name("test_msg_hello_world");
        std::shared_ptr<hello_world_body> msg_body = std::make_shared<hello_world_body>();
        msg_body->m_hello_world = "hello world";
        msg->m_body = msg_body;

        MSG_BUS_PUB<int32_t, std::shared_ptr<message>>("test_msg_hello_world", msg);

        std::this_thread::sleep_for(2s);
    }

    return ERR_SUCCESS;
}