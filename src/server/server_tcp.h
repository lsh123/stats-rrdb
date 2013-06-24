/*
 * server_tcp.h
 *
 *  Created on: Jun 5, 2013
 *      Author: aleksey
 */

#ifndef SERVER_TCP_H_
#define SERVER_TCP_H_

#include <cstddef>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>

#include "common/types.h"

class config;
class thread_pool;
class rrdb;
class connection_tcp;

class server_tcp
{
  friend class connection_tcp;

public:
  server_tcp(boost::shared_ptr<rrdb> rrdb);
  virtual ~server_tcp();

  void initialize(boost::asio::io_service& io_service, boost::shared_ptr<config> config);

  void start();
  void stop();

  void update_status(const time_t & now);

protected:
  void accept();

  void handle_accept(
      const boost::intrusive_ptr<connection_tcp> & new_connection,
      const boost::system::error_code& error
  );
  void handle_read(
      const boost::intrusive_ptr<connection_tcp> & new_connection,
      const boost::system::error_code& error,
      my::size_t bytes_transferred
  );

  inline boost::shared_ptr<rrdb> get_rrdb() const {
    return _rrdb;
  }

private:
  boost::shared_ptr<thread_pool>                    _thread_pool;
  boost::shared_ptr<rrdb>                           _rrdb;
  boost::shared_ptr<boost::asio::ip::tcp::acceptor> _acceptor;

  std::string  _address;
  int          _port;
  my::size_t  _thread_pool_size;
  my::size_t  _buffer_size;
}; // server_tcp

#endif /* SERVER_TCP_H_ */
