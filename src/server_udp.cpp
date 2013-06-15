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
#include "rrdb/rrdb.h"

using namespace boost::asio::ip;

/**
 * One connection
 */
class connection_udp:
    public thread_pool_task,
    public boost::enable_shared_from_this<connection_udp>
{
public:
  connection_udp(
      const boost::shared_ptr<boost::asio::ip::udp::socket> & socket,
      const boost::shared_ptr<rrdb> & rrdb,
      std::size_t buffer_size
  ) :
    _socket(socket),
    _rrdb(rrdb)
  {
    _input_buffer.resize(buffer_size);
  }

  virtual ~connection_udp()
  {
  }

  udp::endpoint & get_endpoint()
  {
    return _remote_endpoint;
  }

  std::string & get_input_buffer()
  {
    return _input_buffer;
  }

public:
  // thread_pool_task
  void run() {
    memory_buffer_t res(_output_buffer);
    try {
        _rrdb->execute_short_command(_input_buffer, res);
    } catch(std::exception & e) {
        LOG(log::LEVEL_ERROR, "Exception executing short rrdb command: %s", e.what());

        res.clear();
        res << "ERROR: " << e.what();
    } catch(...) {
        LOG(log::LEVEL_ERROR, "Unknown exception executing short rrdb command");

        res.clear();
        res << "ERROR: " << "unknown exception";
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

    // TODO:
    // do we bother to send UDP responses?
    // if(!_send_response) {
    //    return;
    // }
    _socket->async_send_to(
        boost::asio::buffer(_output_buffer),
        _remote_endpoint,
        boost::bind(
          &connection_udp::handle_send,
          shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred
        )
    );
  }

  void handle_send(
      const boost::system::error_code& error,
      std::size_t bytes_transferred
  ) {
    // any errors?
    if (error) {
        LOG(log::LEVEL_ERROR, "UDP Server send failed - %d: %s", error.value(), error.message().c_str());
    }

    // log
    LOG(log::LEVEL_DEBUG3, "UDP Server sent %lu bytes", SIZE_T_CAST bytes_transferred);

    // do nothing for now
  }


private:
  boost::shared_ptr<boost::asio::ip::udp::socket> _socket;
  boost::shared_ptr<rrdb>                         _rrdb;

  udp::endpoint                 _remote_endpoint;
  std::string                   _input_buffer;
  memory_buffer_data_t          _output_buffer;
}; // class connection_udp


server_udp::server_udp(
    boost::asio::io_service& io_service,
    boost::shared_ptr<rrdb> rrdb,
    boost::shared_ptr<config> config
) :
  _rrdb(rrdb),
  _address(config->get<std::string>("server_udp.address", "0.0.0.0")),
  _port(config->get<int>("server_udp.port", 9876)),
  _thread_pool_size(config->get<std::size_t>("server_udp.thread_pool_size", 5)),
  _buffer_size(config->get<std::size_t>("server_udp.max_message_size", 2048)),
  _send_response(config->get<bool>("server_udp.send_response", false))
{
  // log
  LOG(log::LEVEL_DEBUG, "Starting UDP server on %s:%d", _address.c_str(), _port);

  // create socket
  _socket.reset(new udp::socket(io_service, udp::endpoint(address_v4::from_string(_address), _port)));
  if(!_socket->is_open()) {
      throw exception("Unable to listen to UDP %s:%d", _address.c_str(), _port);
  }

  // done
  LOG(log::LEVEL_INFO, "Started UDP server on %s:%d", _address.c_str(), _port);
}

server_udp::~server_udp()
{
}

void server_udp::start()
{
  // create threads
  _thread_pool.reset(new thread_pool(_thread_pool_size));

  //
  this->receive();
}


void server_udp::stop()
{
  LOG(log::LEVEL_DEBUG, "Stopping UDP server");

  if(_socket) {
      _socket->close();
      _socket.reset();
  }
  _thread_pool.reset();

  LOG(log::LEVEL_INFO, "Stopped UDP server");
}

void server_udp::receive()
{
  boost::shared_ptr<connection_udp> new_connection(
      new connection_udp(_socket, _rrdb, _buffer_size)
  );

  _socket->async_receive_from(
      boost::asio::buffer(
          (void*)new_connection->get_input_buffer().c_str(),
          new_connection->get_input_buffer().size()
      ),
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
      LOG(log::LEVEL_ERROR, "UDP Server receive failed - %d: %s", error.value(), error.message().c_str());
      return;
  }

  // log
  LOG(log::LEVEL_DEBUG3, "UDP Server received %lu bytes", SIZE_T_CAST bytes_transferred);

  // offload task for processing to the buffer pool
  new_connection->get_input_buffer().resize(bytes_transferred);
  _thread_pool->run(new_connection);

  // next one, please
  this->receive();
}
