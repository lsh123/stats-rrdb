/*
 * server.h
 *
 *  Created on: Jun 2, 2013
 *      Author: aleksey
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <cstddef>
#include <string>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

class udp_server
{
public:
  udp_server(boost::asio::io_service& io_service);

private:
  void start_receive();

  void handle_receive(
    const boost::system::error_code & error,
    std::size_t bytes_transferred
  );

  void handle_send(
    boost::shared_ptr<std::string> message,
    const boost::system::error_code& error,
    std::size_t bytes_transferred
  );

  std::string make_daytime_string();

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint remote_endpoint_;
  boost::array<char, 1> recv_buffer_;
}; // udp_server

#endif /* SERVER_H_ */
