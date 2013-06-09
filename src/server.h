/*
 * server.h
 *
 *  Created on: Jun 2, 2013
 *      Author: aleksey
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <cstddef>

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

// forward
class config;
class thread_pool;

class rrdb;
class server_udp;
class server_tcp;

class server {
public:
  server(boost::shared_ptr<config> config);
  virtual ~server();

public:
  void run();
  void stop();

private:
  boost::shared_ptr<rrdb>        _rrdb;
  boost::shared_ptr<server_udp>  _server_udp;
  boost::shared_ptr<server_tcp>  _server_tcp;

  boost::asio::io_service        _io_service;
}; // class server

#endif /* SERVER_H_ */
