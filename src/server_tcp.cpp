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
#include "rrdb/rrdb.h"

using namespace boost::asio::ip;

/**
 * One connection
 */
class connection_tcp:
    public thread_pool_task,
    public boost::enable_shared_from_this<connection_tcp>
{
public:
  connection_tcp(boost::asio::io_service& io_service, std::size_t buffer_size) :
    _socket(io_service),
    _buffer(buffer_size)
  {
  }

  virtual ~connection_tcp()
  {
  }

  tcp::socket& get_socket()
  {
    return _socket;
  }

  std::vector<char> & get_buffer()
  {
    return _buffer;
  }

  void set_server(boost::shared_ptr<server_tcp> server_tcp)
  {
    _server_tcp = server_tcp;
  }

public:
  // thread_pool_task
  void run() {
    try {
        rrdb::t_result_buffers res = _server_tcp->get_rrdb()->execute_long_command(_buffer);
        _server_tcp->send_response(_socket, res);
    } catch(std::exception & e) {
        log::write(log::LEVEL_ERROR, "Exception executing long rrdb command: %s", e.what());
        _server_tcp->send_response(_socket, this->get_error_buffers(server_tcp::format_error(e.what())));
    } catch(...) {
        log::write(log::LEVEL_ERROR, "Unknown exception long short rrdb command");
        _server_tcp->send_response(_socket, this->get_error_buffers(server_tcp::format_error("unhandled exception")));
    }
  }

private:
  rrdb::t_result_buffers get_error_buffers(const std::string & error_msg)
  {
    rrdb::t_result_buffers res;
    res.push_back(boost::asio::buffer(error_msg));
    return res;
  }

private:
  tcp::socket       _socket;
  std::vector<char> _buffer;
  boost::shared_ptr<server_tcp> _server_tcp;
}; // class connection_tcp

server_tcp::server_tcp(
    boost::asio::io_service& io_service,
    boost::shared_ptr<rrdb> rrdb,
    boost::shared_ptr<config> config
) :
  _rrdb(rrdb),
  _address(config->get<std::string>("server_tcp.address", "0.0.0.0")),
  _port(config->get<int>("server_tcp.port", 9876)),
  _thread_pool_size(config->get<std::size_t>("server_tcp.thread_pool_size", 5)),
  _buffer_size(config->get<std::size_t>("server_tcp.max_message_size", 4096))
{
  // log
  log::write(log::LEVEL_DEBUG, "Starting TCP server on %s:%d", _address.c_str(), _port);

  // create socket
  _acceptor.reset(new tcp::acceptor(io_service, tcp::endpoint(address_v4::from_string(_address), _port)));
  if(!_acceptor->is_open()) {
      throw exception("Unable to listen to TCP %s:%d", _address.c_str(), _port);
  }

  // done
  log::write(log::LEVEL_INFO, "Started TCP server on %s:%d", _address.c_str(), _port);
}

server_tcp::~server_tcp()
{
}

void server_tcp::start()
{
  // create threads
  _thread_pool.reset(new thread_pool(_thread_pool_size));

  this->accept();
}


void server_tcp::stop()
{
  log::write(log::LEVEL_DEBUG, "Stopping TCP server");

  if(_acceptor) {
      _acceptor->close();
      _acceptor.reset();
  }
  _thread_pool.reset();

  log::write(log::LEVEL_INFO, "Stopped TCP server");
}

void server_tcp::accept()
{
  boost::shared_ptr<connection_tcp> new_connection(new connection_tcp(_acceptor->get_io_service(), _buffer_size));

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
  log::write(log::LEVEL_DEBUG3, "TCP Server accepted new connection");

  // start async read
  async_read(
      new_connection->get_socket(),
      boost::asio::buffer(new_connection->get_buffer()),
      boost::bind(
          &server_tcp::handle_read,
          this,
          new_connection,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred
      )
  );

  // next one, please
  this->accept();
}

void server_tcp::handle_read(
    boost::shared_ptr<connection_tcp> new_connection,
    const boost::system::error_code& error,
    std::size_t bytes_transferred
) {
  // any errors?
  if (error && error != boost::asio::error::eof) {
      log::write(log::LEVEL_ERROR, "TCP Server read failed - %d: %s", error.value(), error.message().c_str());
      return;
  } else if(error != boost::asio::error::eof) {
      log::write(log::LEVEL_ERROR, "TCP Server received request larger than the buffer size %zu bytes, consider increasing server.tcp_max_message_size config parameter", _buffer_size);
      return;
  }

  // log
  log::write(log::LEVEL_DEBUG3, "TCP Server read %zu bytes", bytes_transferred);

  // off-load task for processing to the buffer pool
  new_connection->get_buffer().resize(bytes_transferred);
  new_connection->set_server(shared_from_this());
  _thread_pool->run(new_connection);
}

void server_tcp::send_response(
     boost::asio::ip::tcp::socket & socket,
     const std::vector<boost::asio::const_buffer> & buffers
) {

  if(buffers.size() == 0) {
      return;
  }

  boost::asio::async_write(
      socket,
      buffers,
      boost::bind(
          &server_tcp::handle_write,
          this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred
      )
  );
}

void server_tcp::handle_write(
    const boost::system::error_code& error,
    std::size_t bytes_transferred
) {
  // any errors?
  if (error) {
    log::write(log::LEVEL_ERROR, "TCP Server write failed - %d: %s", error.value(), error.message().c_str());
  }

  // log
  log::write(log::LEVEL_DEBUG3, "TCP Server sent %zu bytes", bytes_transferred);

  // do nothing
}

std::string server_tcp::format_error(const char * what) {
  return std::string("ERROR - ") + what;
}
