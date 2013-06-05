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
class server_udp;

class server {
public:
  server(boost::shared_ptr<config> config);
  virtual ~server();

public:
  void run();

private:
  boost::shared_ptr<config>      _config;
  boost::shared_ptr<thread_pool> _thread_pool;
  boost::shared_ptr<server_udp>  _server_udp;

  boost::asio::io_service        _io_service;

}; // class server

#endif /* SERVER_H_ */
