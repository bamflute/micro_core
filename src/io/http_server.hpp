#pragma once

#include <functional>
#include <io/http_macro.hpp>
#include <thread/uv_thread_pool.hpp>

#ifndef _WIN32
#include <unistd.h>
#endif

#define MAX_WRITE_HANDLES           1000

namespace micro
{
    namespace core
    {

        class http_server
        {
        public:

            typedef std::function<void(http_req&, http_rsp&)> svc_functor;

            template<typename Type>
            friend void attachEvents(Type* instance, http_parser_settings& settings);

            http_server(svc_functor functor) : m_functor(functor)
            {
                m_pool.init();
                m_pool.start();
            }

            ~http_server() = default;

            int32_t listen(const std::string &ip, uint16_t port)
            {
                static http_parser_settings settings;
                attachEvents(this, settings);

                int status = 0;

#ifdef _WIN32
                SYSTEM_INFO sysinfo;
                GetSystemInfo(&sysinfo);
                int cores = sysinfo.dwNumberOfProcessors;
#else
                int cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

                std::stringstream cores_string;
                cores_string << cores;
                std::string str;

#ifdef _WIN32
                SetEnvironmentVariableA("UV_THREADPOOL_SIZE", cores_string.str().c_str());
#else
                setenv("UV_THREADPOOL_SIZE", cores_string.str().c_str(), 1);
#endif

                struct sockaddr_in address;

                static std::function<void(uv_stream_t* socket, int status)> on_connect;
                static std::function<void(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)> read;

                //m_loop = uv_default_loop();
                m_loop = m_pool.get_loop();
                uv_tcp_init(m_loop, &m_socket);

                //
                // @TODO - Not sure exactly how to use this,
                // after the initial timeout, it just
                // seems to kill the server.
                //
                //uv_tcp_keepalive(&socket_,1,60);

                status = uv_ip4_addr(ip.c_str(), port, &address);
                ASSERT_STATUS(status, "Resolve Address");

                status = uv_tcp_bind(&m_socket, (const struct sockaddr*) &address, 0);
                ASSERT_STATUS(status, "Bind");

                // called once a connection is made.
                on_connect = [&](uv_stream_t* handle, int status)
                {
                    http_context * context = new http_context();

                    // init tcp handle
                    uv_tcp_init(m_loop, &context->m_handle);

                    // init http parser
                    http_parser_init(&context->m_parser, HTTP_REQUEST);

                    // client reference for parser routines
                    context->m_parser.data = context;

                    // client reference for handle data on requests
                    context->m_handle.data = context;

                    // accept connection passing in refernce to the client handle
                    uv_accept(handle, (uv_stream_t*)&context->m_handle);

                    // called for every read
                    read = [&](uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) 
                    {
                        ssize_t parsed;
                        http_context * context = static_cast<http_context *>(tcp->data);

                        if (nread >= 0) 
                        {
                            parsed = (ssize_t)http_parser_execute(&context->m_parser,
                                &settings,
                                buf->base,
                                nread);

                            // close handle
                            if (parsed < nread) 
                            {
                                uv_close((uv_handle_t*)&context->m_handle, free_context);
                            }
                        }
                        else 
                        {
                            if (nread != UV_EOF)
                            {
                                // @TODO - debug error
                            }

                            // close handle
                            uv_close((uv_handle_t*)&context->m_handle, free_context);
                        }

                        // free request buffer data
                        free(buf->base);
                    };

                    // allocate memory and attempt to read.
                    uv_read_start((uv_stream_t*)&context->m_handle,
                        // allocator
                        [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) 
                        {
                            *buf = uv_buf_init((char*)malloc(suggested_size), (unsigned int)suggested_size);
                        },

                        // reader
                        [](uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) 
                        {
                            read(tcp, nread, buf);
                        });
                };

                status = uv_listen((uv_stream_t*)&m_socket, MAX_WRITE_HANDLES,
                    // listener
                    [](uv_stream_t* socket, int status) 
                    {
                        on_connect(socket, status);
                    });

                ASSERT_STATUS(status, "Listen");

                // init loop
                //uv_run(m_loop, UV_RUN_DEFAULT);
                return 0;
            }

            int complete(http_parser* parser, svc_functor functor)
            {
                http_context * context = reinterpret_cast<http_context *>(parser->data);
                http_req req;
                http_rsp rsp;

                req.m_url = context->m_url;
                req.m_method = context->m_method;
                req.m_headers = context->m_headers;
                req.m_body.swap(context->m_body);                

                rsp.m_parser = *parser;

                functor(req, rsp);
                return 0;
            }

        protected:

            uv_thread_pool m_pool;

            uv_loop_t* m_loop;

            uv_tcp_t m_socket;

            svc_functor m_functor;

        };

    }

}