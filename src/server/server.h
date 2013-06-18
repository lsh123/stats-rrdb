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
#include <boost/enable_shared_from_this.hpp>

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>


#include "spinlock.h"

// forward
class config;
class thread_pool;

class rrdb;
class server_udp;
class server_tcp;
class server_status;

class server :
    public boost::enable_shared_from_this<server>
{
public:
  server();
  virtual ~server();

  void initialize(boost::shared_ptr<config> config);
  void daemonize(const std::string & pid_file = std::string());
  void setuid_user(const std::string & user);

  void run();
  void stop();

  boost::shared_ptr<const server_status> get_status();


  void test(const std::string & params);

private:
  void signal_handler(const boost::system::error_code& error, int signal_number);
  void notify_before_fork();
  void notify_after_fork(bool is_parent);

private:
  boost::asio::io_service          _io_service;

  boost::shared_ptr<const server_status> _server_status;
  spinlock                         _server_status_lock;

  boost::shared_ptr<rrdb>          _rrdb;
  boost::shared_ptr<server_udp>    _server_udp;
  boost::shared_ptr<server_tcp>    _server_tcp;

  boost::asio::signal_set          _exit_signals;
}; // class server

#endif /* SERVER_H_ */
