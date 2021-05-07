#pragma once


#include <boost/asio.hpp>


namespace micro
{
    namespace core
    {

        class socket_address
        {
        public:

            socket_address() = default;

            virtual ~socket_address() = default;
            
            typedef boost::asio::ip::tcp::endpoint ep_type;

            ep_type  get() const { return m_ep; }
        
            socket_address(const std::string &ip, uint16_t port) : m_ip(ip), m_port(port),m_ep(boost::asio::ip::address::from_string(ip), port) {}

            socket_address(const ep_type &ep) : m_ip(ep.address().to_string()), m_port(ep.port()) {}

            socket_address& operator=(const ep_type &ep)
            {
                m_ip = ep.address().to_string();
                m_port = ep.port();
                return *this;
            }

            bool operator==(const socket_address &addr)
            {
                return ((m_ip == addr.get_ip()) && (m_port == addr.get_port()));
            }

            uint16_t get_port() const { return m_port; }
            std::string get_ip() const {return m_ip;}

        protected:

            ep_type m_ep;

            std::string m_ip;

            uint16_t m_port;

        };

    }

}
