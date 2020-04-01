#include "test_http.h"
#include <thread>

using namespace micro::core;
using namespace std::chrono_literals;

void test_http_client(int argc, char* argv[])
{
    http_client client("http://www.baidu.com", 
        [](auto &res) 
    {
        std::cout << res.m_body << std::endl;
    });
}

void test_http_server(int argc, char* argv[])
{
    http_server server([](auto &req, auto &res)
    {
        res.set_status(200);
        res.set_header("Content-Type", "text/plain");
        res.set_header("Connection", "keep-alive");

        res << "HTTP/1.1 200 OK\r\n\r\n<html><body><h1>我的第一个标题</h1></body></html>" << std::endl;

        res.end();
    });

    server.listen("0.0.0.0", 8000);

    //http_client client("http://localhost:8000",
    http_client client("http://www.baidu.com",
        [](auto &res)
    {
        std::cout << res.m_body << std::endl;
    });

    client.connect();

    while (true)
    {
        std::this_thread::sleep_for(2s);
    }
}
