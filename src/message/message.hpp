#pragma once


#include <string>
#include <memory>
#include <common/common.hpp>




namespace micro
{
    namespace core
    {

        enum channel_type
        {
            SERVER_TYPE = 0,
            CLIENT_TYPE = 1,
            IPC_TYPE        = 2,
            INNER_TYPE  = 3
        };

        class channel_source
        {
        public:

            channel_source() : m_channel_type(INNER_TYPE), m_channel_id(0) {}

            channel_source(channel_type type, uint64_t id) : m_channel_type(type), m_channel_id(id) {}

            std::string to_string() const
            {
                std::stringstream stream;
                stream << " channel source: " << m_channel_type << "-" << m_channel_id;
                return  stream.str();
            }

            uint64_t m_channel_id;
            uint32_t m_channel_type;
        };

        class base_header
        {
        public:

            typedef channel_source channel_type;

            base_header() : m_name(""), m_priority(0) {}

            std::string m_name;
            uint32_t    m_priority;

            channel_type m_src;
            channel_type m_dst;
        };

        class base_body
        {
        public:
            virtual ~base_body() = default;
        };

        class message
        {
        public:

            typedef std::shared_ptr<base_header> header_ptr_type;
            typedef std::shared_ptr<base_body>    body_ptr_type;

            message() : m_header(std::make_shared<base_header>()) {}

            virtual std::string get_name() const { return m_header->m_name; }
            virtual void set_name(const std::string &name) { m_header->m_name = name; }

            virtual uint32_t get_priority() const { return m_header->m_priority; }
            virtual void set_priority(uint32_t priority) { m_header->m_priority = priority; }

            header_ptr_type m_header;
            body_ptr_type     m_body;
        };

    }

}
