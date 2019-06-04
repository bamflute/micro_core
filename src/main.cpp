#include <common/error.hpp>
#include <logger/logger.hpp>
#include <module/base_module.hpp>
#include <thread/nio_thread_pool.hpp>
#include <timer/timer_generator.hpp>
#include <module/module.hpp>
#include <bus/message_bus.hpp>
#include <boost/serialization/singleton.hpp>



    void hello_world(const int & a)
    {
        std::cout << "a: " << a << std::endl;
    }


int main(int argc, char* argv[])
{
    int a = 10;
    MSG_BUS_SUB("hello_world", &hello_world);
    MSG_BUS_PUB<void, const int &>("hello_world", a);

    return ERR_SUCCESS;
}