/*
 * server.cpp
 *
 *  Created on: Jun 2, 2013
 *      Author: aleksey
 */
#include "server.h"

#include <iostream>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#include "log.h"
#include "config.h"


using boost::asio::ip::udp;


udp_server::udp_server(boost::asio::io_service& io_service, std::size_t recv_buffer_size) :
  socket_(io_service, udp::endpoint(udp::v4(), 9876)),
  _recv_buffer(recv_buffer_size)
{
  start_receive();
}

void udp_server::start_receive()
{
  socket_.async_receive_from(
      boost::asio::buffer(_recv_buffer),
      remote_endpoint_,
      boost::bind(
          &udp_server::handle_receive, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred
      )
  );
}

void udp_server::handle_receive(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  if (!error || error == boost::asio::error::message_size)
  {
    boost::shared_ptr<std::string> message(
        new std::string("aaa"));

    socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
        boost::bind(&udp_server::handle_send, this, message,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));

    std::cout << "Received: " << bytes_transferred << std::endl;

    start_receive();
  }
}

void udp_server::handle_send(boost::shared_ptr<std::string> message,
    const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  std::cout << "Sent: " << bytes_transferred << std::endl;
}

