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
#include <boost/enable_shared_from_this.hpp>

class config;
class thread_pool;

class connection_tcp;

class server_tcp :
    public boost::enable_shared_from_this<server_tcp>
{
  friend class connection_tcp;

public:
  server_tcp(
      boost::asio::io_service& io_service,
      boost::shared_ptr<thread_pool> thread_pool,
      boost::shared_ptr<config> config
  );
  virtual ~server_tcp();

  void start_accept();

protected:
  void handle_accept(boost::shared_ptr<connection_tcp> new_connection, const boost::system::error_code& error);
  void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

private:
  boost::shared_ptr<thread_pool>                      _thread_pool;
  boost::shared_ptr<boost::asio::ip::tcp::acceptor>   _acceptor;
}; // server_tcp

#endif /* SERVER_TCP_H_ */
