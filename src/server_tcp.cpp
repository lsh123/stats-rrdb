/*
 * server_tcp.cpp
 *
 *  Created on: Jun 5, 2013
 *      Author: aleksey
 */

#include "server_tcp.h"

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
class connection_tcp:
    public thread_pool_task,
    public boost::enable_shared_from_this<connection_tcp>
{
public:
  connection_tcp(boost::asio::io_service& io_service) :
    _socket(io_service)
  {
  }

  virtual ~connection_tcp()
  {
  }

  tcp::socket& get_socket()
  {
    return _socket;
  }

  void set_server(boost::shared_ptr<server_tcp> server_tcp)
  {
    _server_tcp = server_tcp;
  }

public:
  // thread_pool_task
  void run() {
    boost::asio::async_write(
        _socket,
        boost::asio::buffer("test"),
        boost::bind(
            &server_tcp::handle_write,
            _server_tcp,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        )
    );
  }

private:
  tcp::socket _socket;
  boost::shared_ptr<server_tcp> _server_tcp;
}; // class connection_tcp

server_tcp::server_tcp(
    boost::asio::io_service& io_service,
    boost::shared_ptr<thread_pool> thread_pool,
    boost::shared_ptr<config> config
) :
  _thread_pool(thread_pool)
{
  // load configs
  std::string tcp_address          = config->get<std::string>("server.tcp_address", "0.0.0.0");
  int         tcp_port             = config->get<int>("server.tcp_port", 9876);

  // create
  _acceptor.reset(new tcp::acceptor(io_service, tcp::endpoint(address_v4::from_string(tcp_address), tcp_port)));
  if(!_acceptor->is_open()) {
      throw exception("Unable to listen to TCP %s:%d", tcp_address.c_str(), tcp_port);
  }

  // done
  log::write(log::LEVEL_INFO, "Listening to TCP %s:%d", tcp_address.c_str(), tcp_port);
}

server_tcp::~server_tcp()
{
}

void server_tcp::start_accept()
{
  boost::shared_ptr<connection_tcp> new_connection(new connection_tcp(_acceptor->io_service()));

  _acceptor->async_accept(
      new_connection->get_socket(),
      boost::bind(
          &server_tcp::handle_accept,
          this,
          new_connection,
          boost::asio::placeholders::error
      )
  );
}

void server_tcp::handle_accept(
    boost::shared_ptr<connection_tcp> new_connection,
    const boost::system::error_code& error
) {
  // any errors?
  if (error) {
      log::write(log::LEVEL_ERROR, "TCP Server accept failed - %d: %s", error.value(), error.message().c_str());
      return;
  }

  // log
  log::write(log::LEVEL_DEBUG, "TCP Server accepted new connection");

  // offload task for processing to the buffer pool
  new_connection->set_server(shared_from_this());
  _thread_pool->run(new_connection);

  // next one, please
  start_accept();
}

void server_tcp::handle_write(
    const boost::system::error_code& error,
    std::size_t bytes_transferred
) {
  // any errors?
  if (!error) {
    log::write(log::LEVEL_ERROR, "TCP Server write failed - %d: %s", error.value(), error.message().c_str());
  }

  // log
  log::write(log::LEVEL_DEBUG, "TCP Server sent %zu bytes", bytes_transferred);

  // do nothing
}

