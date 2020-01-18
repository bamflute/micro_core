#pragma once

#include <functional>
#include <io/http_macro.hpp>
#include <thread/uv_thread_pool.hpp>


namespace micro
{
    namespace core
    {

        class http_client
        {
        public:

            typedef std::function<void(http_rsp & rsp)> svc_functor;

            template<typename Type>
            friend void attachEvents(Type* instance, http_parser_settings& settings);

            struct Options 
            {
                std::string host = "localhost";

                int port = 80;

                std::string method = "GET";

                std::string url = "/";
            };

            http_client(Options o, svc_functor functor) : m_functor(functor)
            {
                m_opts = o;

                m_pool.init();
                m_pool.start();

                //connect();
            }

            http_client(std::string ustr, svc_functor functor) : m_functor(functor)
            {
                auto u = uri::ParseHttpUrl(ustr);

                m_opts.host = u.host;

                if (u.port) 
                {
                    m_opts.port = u.port;
                }

                if (!u.path.empty()) 
                {
                    m_opts.url = u.path;
                }

                m_pool.init();
                m_pool.start();

                //connect();
            }

            ~http_client() {}

            void connect()
            {
                struct addrinfo ai;
                ai.ai_family = PF_INET;
                ai.ai_socktype = SOCK_STREAM;
                ai.ai_protocol = IPPROTO_TCP;
                ai.ai_flags = 0;

                m_loop = m_pool.get_loop();

                static std::function<void(uv_getaddrinfo_t* req, int status, struct addrinfo* res)> on_resolved;

                static std::function<void(uv_connect_t* req, int status)> on_before_connect;

                on_before_connect = [&](uv_connect_t* req, int status) 
                {

                    // @TODO
                    // Populate address and time info for logging / stats etc.

                    on_connect(req, status);
                };

                on_resolved = [&](uv_getaddrinfo_t* req, int status, struct addrinfo* res) 
                {

                    char addr[17] = { '\0' };

                    uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
                    uv_freeaddrinfo(res);

                    struct sockaddr_in dest;
                    uv_ip4_addr(addr, m_opts.port, &dest);

                    http_context * context = new http_context();

                    context->m_handle.data = context;
                    http_parser_init(&context->m_parser, HTTP_RESPONSE);
                    context->m_parser.data = context;

                    uv_tcp_init(m_loop, &context->m_handle);
                    //uv_tcp_keepalive(&context->handle, 1, 60);

                    uv_tcp_connect(
                        &context->m_connect_req,
                        &context->m_handle,
                        (const struct sockaddr*) &dest,
                        [](uv_connect_t* req, int status) 
                    {
                        on_before_connect(req, status);
                    });
                };

                auto cb = [](uv_getaddrinfo_t* req, int status, struct addrinfo* res) 
                {
                    on_resolved(req, status, res);
                };

                uv_getaddrinfo(m_loop, &m_addr_req, cb, m_opts.host.c_str(), std::to_string(m_opts.port).c_str(), &ai);
                //uv_run(m_loop, UV_RUN_DEFAULT);

            }

            int complete(http_parser* parser, svc_functor functor)
            {
                http_context * context = reinterpret_cast<http_context *>(parser->data);

                http_rsp rsp;
                rsp.m_body = context->m_body.str();
                rsp.m_parser = *parser;

                functor(rsp);
                return 0;
            }

            void on_connect(uv_connect_t* req, int status)
            {
                http_context* context = reinterpret_cast<http_context *>(req->handle->data);
                static http_parser_settings settings;
                attachEvents(this, settings);

                if (status == -1) {
                    // @TODO
                    // Error Callback
                    uv_close((uv_handle_t*)req->handle, free_context);
                    return;
                }

                static std::function<void(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)> read;

                read = [&](uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) 
                {

                    http_context * context = static_cast<http_context *>(tcp->data);

                    if (nread >= 0) 
                    {
                        auto parsed = (ssize_t)http_parser_execute(&context->m_parser, &settings, buf->base, nread);

                        if (parsed < nread) 
                        {
                            uv_close((uv_handle_t*)&context->m_handle, free_context);
                        }

                        if (parsed != nread) 
                        {
                            // @TODO
                            // Error Callback
                        }
                    }
                    else 
                    {
                        if (nread != UV_EOF) 
                        {
                            return; // maybe do something interesting here...
                        }

                        uv_close((uv_handle_t*)&context->m_handle, free_context);
                    }

                    free(buf->base);
                };

                uv_buf_t req_buf;
                std::string reqstr = m_opts.method + " " + m_opts.url + " HTTP/1.1" + CRLF + 
                    //
                    // @TODO
                    // Add user's headers here
                    //
                    "Connection: keep-alive" + CRLF + CRLF;

                req_buf.base = (char*)reqstr.c_str();
                req_buf.len = (unsigned long)reqstr.size();

                uv_read_start(
                    req->handle,
                    [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) 
                    {
                        *buf = uv_buf_init((char*)malloc(suggested_size), (unsigned int)suggested_size);
                    },
                    [](uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) 
                    {
                        read(tcp, nread, buf);
                    });

                uv_write(
                    &context->m_write_req,
                    req->handle,
                    &req_buf,
                    1,
                    NULL);
            }

        protected:

            Options m_opts;

            uv_thread_pool m_pool;

            uv_loop_t * m_loop;

            uv_tcp_t m_socket;

            svc_functor m_functor;

            uv_getaddrinfo_t m_addr_req;

            uv_shutdown_t m_shutdown_req;

        };

    }

}
