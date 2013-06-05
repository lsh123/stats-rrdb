/*
 * command_udp.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef COMMAND_UDP_H_
#define COMMAND_UDP_H_

#include "command.h"
#include <boost/asio.hpp>

class server_udp;

class command_udp:
    public command
{
public:
  command_udp(boost::shared_ptr<server_udp> server_udp, const boost::asio::ip::udp::endpoint & remote_endpoint);
  virtual ~command_udp();

public:
  // command
  void send_response(const std::string & message);

private:
  boost::shared_ptr<server_udp>  _server_udp;
  boost::asio::ip::udp::endpoint _remote_endpoint;
};

#endif /* COMMAND_UDP_H_ */
