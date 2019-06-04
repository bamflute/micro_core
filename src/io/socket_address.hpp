#pragma once


#include <boost/asio.hpp>


namespace micro
{
    namespace core
    {

        class socket_address
        {
        public:

            typedef boost::asio::ip::tcp::endpoint ep_type;

            socket_address(std::string ip, uint16_t port)
                : m_ip(ip)
                , m_port(port)
                , m_ep(boost::asio::ip::address::from_string(ip), port)
            {}

            ep_type  get() const { return m_ep; }

        protected:

            ep_type m_ep;

            std::string m_ip;

            uint16_t m_port;

        };

    }

}
