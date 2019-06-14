#include <test/test_io.h>
#include <io/bootstrap.hpp>


int test_io(int argc, char* argv[])
{
    //bootstrap group
    BOOTSTRAP_POOL(pool, 1);
    //std::shared_ptr<nio_thread_pool> pool = std::make_shared<nio_thread_pool>();
    //pool->init(1);
    //pool->start();

    //bootstrap acceptor
    BOOTSTRAP_ACCEPTOR(acceptor, "127.0.0.1", 9999, pool, pool, default_initializer, echo_initializer, echo_initializer);
    //boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 9999);
    //std::shared_ptr<tcp_acceptor> acceptor = std::make_shared<tcp_acceptor>(pool->get_ios(), endpoint);

    //acceptor->group(pool, pool);
    //acceptor->channel_initializer(std::make_shared<echo_initializer>(), std::make_shared<echo_initializer>());

    //acceptor->init();

    //bootstrap connector
    BOOTSTRAP_CONNECTOR(connector, "127.0.0.1", 9999, pool, pool, echo_connector_initializer, echo_initializer, echo_initializer);
    //std::shared_ptr < tcp_connector> connector = std::make_shared<tcp_connector>();
    //
    //connector->group(pool, pool);
    //connector->connector_initializer(std::make_shared<echo_connector_initializer>());
    //connector->channel_initializer(std::make_shared<echo_initializer>(), std::make_shared<echo_initializer>());

    //connector->init();

    //connector->connect(endpoint);

    while (true)
    {
        
    }
    
    return 0;
}