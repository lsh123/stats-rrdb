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

#include "exception.h"
#include "log.h"
#include "config.h"
#include "thread_pool.h"


using namespace boost::asio::ip;

/**
 * One connection
 */
class connection_udp:
    public thread_pool_task,
    public boost::enable_shared_from_this<connection_udp>
{
public:
  connection_udp(std::size_t buffer_size):
    _buffer(buffer_size)
  {
  }

  virtual ~connection_udp()
  {
  }

  udp::endpoint & get_endpoint()
  {
    return _remote_endpoint;
  }

  std::vector<char> & get_buffer()
  {
    return _buffer;
  }

  void set_server(boost::shared_ptr<server_udp> server_udp)
  {
    _server_udp = server_udp;
  }

public:
  // thread_pool_task
  void run() {
    // TODO: test
    std::string str(_buffer.begin(), _buffer.end());
    log::write(log::LEVEL_DEBUG, "UDP Server received %zu bytes buffer: %s", str.size(), str.c_str());

    // done
    _server_udp->send_response(_remote_endpoint, "OK");
  }

private:
  udp::endpoint                 _remote_endpoint;
  std::vector<char>             _buffer;
  boost::shared_ptr<server_udp> _server_udp;
}; // class connection_udp


server_udp::server_udp(
    boost::asio::io_service& io_service,
    boost::shared_ptr<thread_pool> thread_pool,
    boost::shared_ptr<config> config
) :
  _thread_pool(thread_pool)
{
  // load configs
  std::string udp_address = config->get<std::string>("server.udp_address", "0.0.0.0");
  int         udp_port    = config->get<int>("server.udp_port", 9876);
  _buffer_size            = config->get<std::size_t>("server.udp_max_message_size", 2048);

  // create
  _socket.reset(new udp::socket(io_service, udp::endpoint(address_v4::from_string(udp_address), udp_port)));
  if(!_socket->is_open()) {
      throw exception("Unable to listen to UDP %s:%d", udp_address.c_str(), udp_port);
  }

  // done
  log::write(log::LEVEL_INFO, "Listening to UDP %s:%d", udp_address.c_str(), udp_port);
}

server_udp::~server_udp()
{

}

void server_udp::start_receive()
{
  boost::shared_ptr<connection_udp> new_connection(new connection_udp(_buffer_size));

  _socket->async_receive_from(
      boost::asio::buffer(new_connection->get_buffer()),
      new_connection->get_endpoint(),
      boost::bind(
          &server_udp::handle_receive,
          this,
          new_connection,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred
      )
  );
}

void server_udp::handle_receive(
    boost::shared_ptr<connection_udp> new_connection,
    const boost::system::error_code& error,
    std::size_t bytes_transferred
) {
  // any errors?
  if (error) {
      log::write(log::LEVEL_ERROR, "UDP Server receive failed - %d: %s", error.value(), error.message().c_str());
      return;
  }

  // log
  log::write(log::LEVEL_DEBUG, "UDP Server received %zu bytes", bytes_transferred);

  // offload task for processing to the buffer pool
  new_connection->get_buffer().resize(bytes_transferred);
  new_connection->set_server(shared_from_this());
  _thread_pool->run(new_connection);

  // next one, please
  start_receive();
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
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred
      )
  );
}

void server_udp::handle_send(
    const boost::system::error_code& error,
    std::size_t bytes_transferred
) {
  // any errors?
  if (error) {
      log::write(log::LEVEL_ERROR, "UDP Server send failed - %d: %s", error.value(), error.message().c_str());
  }

  // log
  log::write(log::LEVEL_DEBUG, "UDP Server sent %zu bytes", bytes_transferred);

  // do nothing for now
}
