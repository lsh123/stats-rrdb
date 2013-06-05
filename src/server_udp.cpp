/*
 * server_udp.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#include "server_udp.h"

#include <iostream>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#include "log.h"
#include "config.h"
#include "thread_pool.h"

#include "command_udp.h"


using namespace boost::asio::ip;

server_udp::server_udp(
    boost::shared_ptr<thread_pool> thread_pool,
    boost::asio::io_service& io_service,
    boost::shared_ptr<config> config
) :
  _thread_pool(thread_pool)
{
  // load configs
  std::string udp_address          = config->get<std::string>("server.udp_address", "0.0.0.0");
  int         udp_port             = config->get<int>("server.udp_port", 9876);
  std::size_t udp_max_message_size = config->get<std::size_t>("server.udp_max_message_size", 2048);

  // create
  _recv_buffer.resize(udp_max_message_size);
  _socket.reset(new udp::socket(io_service, udp::endpoint(address_v4::from_string(udp_address), udp_port)));

  // start
  start_receive();

  // done
  log::write(log::LEVEL_INFO, "Listening to UDP %s:%d", udp_address.c_str(), udp_port);
}

server_udp::~server_udp()
{

}

void server_udp::start_receive()
{
  _socket->async_receive_from(
      boost::asio::buffer(_recv_buffer),
      _remote_endpoint,
      boost::bind(
          &server_udp::handle_receive, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred
      )
  );
}

void server_udp::send_response(
   const boost::asio::ip::udp::endpoint & endpoint,
   const std::string & message
) {

  _socket->async_send_to(
      boost::asio::buffer(message),
      endpoint,
      boost::bind(
        &server_udp::handle_send,
        this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred
      )
  );
}

void server_udp::handle_receive(
    const boost::system::error_code& error,
    std::size_t bytes_transferred
) {
  if (!error || error == boost::asio::error::message_size)
  {
    log::write(log::LEVEL_DEBUG, "UDP Server received %zu bytes", bytes_transferred);

    boost::shared_ptr<command_udp> cmd(new command_udp(shared_from_this(), _remote_endpoint));
    _thread_pool->run(cmd);

    start_receive();
  }
}

void server_udp::handle_send(
    const boost::system::error_code& error,
    std::size_t bytes_transferred
) {
  log::write(log::LEVEL_DEBUG, "UDP Server sent %zu bytes", bytes_transferred);
}
