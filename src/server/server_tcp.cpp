/*
 * server_tcp.cpp
 *
 *  Created on: Jun 5, 2013
 *      Author: aleksey
 */

#include "server/server_tcp.h"

#include <iostream>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#include "exception.h"
#include "log.h"
#include "config.h"

#include "server/thread_pool.h"

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
  connection_tcp(boost::asio::io_service& io_service, const boost::shared_ptr<rrdb> & rrdb, std::size_t buffer_size) :
    _socket(io_service),
    _rrdb(rrdb),
    _input_buffer(buffer_size)
  {
  }

  virtual ~connection_tcp()
  {
  }

  tcp::socket & get_socket()
  {
    return _socket;
  }

  memory_buffer_data_t & get_input_buffer()
  {
    return _input_buffer;
  }

public:
  // thread_pool_task
  void run() {
    // execute command
    memory_buffer_t res(_output_buffer);
    try {
        _rrdb->execute_tcp_command(_input_buffer, res);
    } catch(std::exception & e) {
        LOG(log::LEVEL_ERROR, "Exception executing long rrdb command: %s", e.what());

        res.clear();
        res << "ERROR: " << e.what();
    } catch(...) {
        LOG(log::LEVEL_ERROR, "Unknown exception long short rrdb command");

        res.clear();
        res << "ERROR: " << "unhandled exception";
    }
    res.flush();

    // add default OK
    if(_output_buffer.empty()) {
        res << "OK";
        res.flush();
    }

    // clear input data
    _input_buffer.clear();
    _rrdb.reset();

    // send
    boost::asio::async_write(
        _socket,
        boost::asio::buffer(_output_buffer),
        boost::bind(
            &connection_tcp::handle_write,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        )
    );
  }

  void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred)
  {
    // any errors?
    if (error) {
      LOG(log::LEVEL_ERROR, "TCP Server write failed - %d: %s", error.value(), error.message().c_str());
    }

    // log
    LOG(log::LEVEL_DEBUG3, "TCP Server sent %lu bytes", SIZE_T_CAST bytes_transferred);

    // do nothing
  }

private:
  tcp::socket                   _socket;
  boost::shared_ptr<rrdb>       _rrdb;
  memory_buffer_data_t          _input_buffer;
  memory_buffer_data_t          _output_buffer;
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
  LOG(log::LEVEL_DEBUG, "Starting TCP server on %s:%d", _address.c_str(), _port);

  // create socket
  _acceptor.reset(new tcp::acceptor(io_service, tcp::endpoint(address_v4::from_string(_address), _port)));
  if(!_acceptor->is_open()) {
      throw exception("Unable to listen to TCP %s:%d", _address.c_str(), _port);
  }

  // done
  LOG(log::LEVEL_INFO, "Started TCP server on %s:%d", _address.c_str(), _port);
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
  LOG(log::LEVEL_DEBUG, "Stopping TCP server");

  if(_acceptor) {
      _acceptor->close();
      _acceptor.reset();
  }
  _thread_pool.reset();

  LOG(log::LEVEL_INFO, "Stopped TCP server");
}

void server_tcp::accept()
{
  boost::shared_ptr<connection_tcp> new_connection(new connection_tcp(_acceptor->get_io_service(), _rrdb, _buffer_size));
  // new_connection->set_rrdb(_rrdb);

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
      LOG(log::LEVEL_ERROR, "TCP Server accept failed - %d: %s", error.value(), error.message().c_str());
      return;
  }

  // log
  LOG(log::LEVEL_DEBUG3, "TCP Server accepted new connection");

  // start async read
  async_read(
      new_connection->get_socket(),
      boost::asio::buffer(new_connection->get_input_buffer()),
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
      LOG(log::LEVEL_ERROR, "TCP Server read failed - %d: %s", error.value(), error.message().c_str());
      return;
  } else if(error != boost::asio::error::eof) {
      LOG(log::LEVEL_ERROR, "TCP Server received request larger than the buffer size %lu bytes, consider increasing server.tcp_max_message_size config parameter", SIZE_T_CAST _buffer_size);
      return;
  }

  // log
  LOG(log::LEVEL_DEBUG3, "TCP Server read %lu bytes", SIZE_T_CAST bytes_transferred);

  // off-load task for processing to the buffer pool
  new_connection->get_input_buffer().resize(bytes_transferred);
  std::size_t used_threads = _thread_pool->run(new_connection);

  // eat our own dog food
  time_t now = time(NULL);
  _rrdb->update_metric("self.tcp.requests", now, 1.0);
  _rrdb->update_metric("self.tcp.requests", now, used_threads);
}

