#pragma once

#include <string>
#include <sstream>
#include <map>
#include <common/common.hpp>
#include <3rd/http_parser/http_parser.h>
#include <3rd/http_parser/uri.h>
__BEGIN_DECLS__
#include "uv.h"
__END_DECLS__

#define ASSERT_STATUS(status, msg) \
  if (status != 0) { \
    std::cerr << msg << ": " << uv_err_name(status); \
    exit(1); \
  }

extern const std::string CRLF;

extern void free_context(uv_handle_t *);


namespace micro
{
    namespace core
    {

        template <class Type>
        extern void attachEvents(Type* instance, http_parser_settings& settings);

        template <class T>
        class http_buffer : public std::stringbuf 
        {

            friend class http_req;
            friend class http_rsp;

            T * m_stream;

            http_buffer<T>(std::ostream& str) {}

            ~http_buffer() = default;

            virtual int sync()
            {
                std::string out = str();
                std::ostringstream buf;

                buf << out;
                out = buf.str();
                m_stream->write_or_end(out, true);
                buf.flush();
                str("");

                return 0;
            }
        };

        template <class Type>
        class http_stream : virtual public std::ostream
        {

        public:

            http_stream() : std::ostream(std::cout.rdbuf()) {}

            //std::stringstream m_stream;

        };

        class http_req
        {
        public:

            std::string m_url;

            std::string m_method;

            std::string m_status_code;

            std::stringstream m_body;

            std::map<const std::string, const std::string> m_headers;
        };

        class http_context : public http_req
        {

        public:

            std::map<int, uv_write_t> m_writes;

            uv_tcp_t m_handle;

            uv_connect_t m_connect_req;

            uv_write_t m_write_req;

            http_parser m_parser;
        };


        class http_rsp : public http_stream<http_rsp>
        {
        public:

            http_rsp() : http_stream<http_rsp>(), std::ostream(m_stream.rdbuf()), m_buffer(m_stream) { m_buffer.m_stream = this; }

            ~http_rsp() {}

            void write_or_end(std::string str, bool end)
            {
                if (m_ended) throw std::runtime_error("Can not write after end");

                std::stringstream ss;

                if (!m_written_or_ended)
                {

                    ss << "HTTP/1.1 " << m_status_code << " " << m_status_adjective << CRLF;

                    for (auto & header : m_headers) 
                    {
                        ss << header.first << ": " << header.second << CRLF;
                    }

                    ss << CRLF;
                    m_written_or_ended = true;
                }

                bool isChunked = m_headers.count("Transfer-Encoding") && m_headers["Transfer-Encoding"] == "chunked";

                if (isChunked) 
                {
                    ss << std::hex << str.size()
                      << std::dec << CRLF << str << CRLF;
                }
                else 
                {
                    ss << str;
                }

                if (isChunked && end) 
                {
                    ss << "0" << CRLF << CRLF;
                }

                str = ss.str();

                //response buffer
                uv_buf_t res_buf;
                res_buf.base = (char*)str.c_str();
                res_buf.len = (unsigned long)str.size();

                http_context * context = static_cast<http_context *>(this->m_parser.data);

                auto id = m_write_count++;

                uv_write_t write_req;
                context->m_writes.insert({ id, write_req });

                if (end)
                {
                    m_ended = true;
                    uv_write(&context->m_writes.at(id), (uv_stream_t*)&context->m_handle, &res_buf, 1,
                        [] (uv_write_t* req, int status)
                        {
                            if (!uv_is_closing((uv_handle_t*)req->handle)) 
                            {
                                uv_close((uv_handle_t*)req->handle, free_context);
                            }
                        }
                    );
                }
                else 
                {
                    uv_write(&context->m_writes.at(id), (uv_stream_t*)&context->m_handle, &res_buf, 1, NULL);
                }
            }

            void set_header(const std::string & key, const std::string & val)
            {
                m_headers_set = true;

                if (m_written_or_ended) throw std::runtime_error("Can not set headers after write");

                if (key == "Content-Length") 
                {
                    m_content_length_set = true;
                }
                
                m_headers.insert({ key, val });
            }

            void set_status(int status_code)
            {
                m_status_set = true;
                if (m_written_or_ended) throw std::runtime_error("Can not set status after write");
                m_status_code = status_code;
            }

            void set_status(int code, std::string ad)
            {
                m_status_set = true;
                if (m_written_or_ended) throw std::runtime_error("Can not set status after write");
                m_status_code = code;
                m_status_adjective = ad;
            }

            void write(const std::string & str)
            {
                this->write_or_end(str, false);
            }

            void end(const std::string & str)
            {
                this->write_or_end(str, true);
            }

            void end()
            {
                this->write_or_end("", true);
            }

        public:

            http_parser m_parser;

            int m_status_code = 200;

            std::string m_body = "";

            std::string m_status_adjective = "OK";

            std::map<const std::string, const std::string> m_headers;

            int m_write_count = 0;

            bool m_written_or_ended = false;

            bool m_ended = false;

            bool m_headers_set = false;

            bool m_status_set = false;

            bool m_content_length_set = false;

            std::stringstream m_stream;

            http_buffer<http_rsp> m_buffer;
        };

    }

}