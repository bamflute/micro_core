#pragma once


#include <io/io_handler.hpp>
#include <io/tcp_channel.hpp>
#include <io/tcp_acceptor.hpp>
#include <io/tcp_connector.hpp>
#include <io/io_handler_initializer.hpp>


#define BOOTSTRAP_ACCEPTOR(ACCEPTOR, IP, PORT, ACCEPTOR_POOL, CHANNEL_POOL, ACCEPTOR_INITIALIZER, INBOUND_INITIALIZER, OUTBOUND_INITIALIZER) \
std::shared_ptr<tcp_acceptor> ACCEPTOR = std::make_shared<tcp_acceptor>(ACCEPTOR_POOL->get_ios(), boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(IP), PORT)); \
ACCEPTOR->group(ACCEPTOR_POOL, CHANNEL_POOL); \
ACCEPTOR->channel_initializer(std::make_shared<INBOUND_INITIALIZER>(), std::make_shared<OUTBOUND_INITIALIZER>()); \
ACCEPTOR->init();


#define BOOTSTRAP_CONNECTOR(CONNECTOR, IP, PORT, CONNECTOR_POOL, CHANNEL_POOL, CONNECTOR_INITIALIZER, INBOUND_INITIALIZER, OUTBOUND_INITIALIZER) \
std::shared_ptr < tcp_connector> CONNECTOR = std::make_shared<tcp_connector>(); \
CONNECTOR->group(CONNECTOR_POOL, CHANNEL_POOL); \
CONNECTOR->connector_initializer(std::make_shared<CONNECTOR_INITIALIZER>()); \
CONNECTOR->channel_initializer(std::make_shared<INBOUND_INITIALIZER>(), std::make_shared<OUTBOUND_INITIALIZER>()); \
CONNECTOR->init(); \
CONNECTOR->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(IP), PORT));