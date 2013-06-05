/*
 * server_udp.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef SERVER_UDP_H_
#define SERVER_UDP_H_

#include <cstddef>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class config;
class thread_pool;

class server_udp : public boost::enable_shared_from_this<server_udp>
{
public:
  server_udp(
      boost::shared_ptr<thread_pool> thread_pool,
      boost::asio::io_service& io_service,
      boost::shared_ptr<config> config
  );
  virtual ~server_udp();

  void send_response(
   const boost::asio::ip::udp::endpoint & endpoint,
   const std::string & message
  );

private:
  void start_receive();

  void handle_receive(
    const boost::system::error_code & error,
    std::size_t bytes_transferred
  );
  void handle_send(
    const boost::system::error_code& error,
    std::size_t bytes_transferred
  );

private:
  boost::shared_ptr<thread_pool> _thread_pool;
  boost::shared_ptr<boost::asio::ip::udp::socket> _socket;
  boost::asio::ip::udp::endpoint _remote_endpoint;
  std::vector<char>              _recv_buffer;
}; // server_udp

#endif /* SERVER_UDP_H_ */
