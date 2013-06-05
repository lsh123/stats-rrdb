/*
 * command_udp.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#include "command_udp.h"
#include "server_udp.h"

command_udp::command_udp(
   boost::shared_ptr<server_udp> server_udp,
   const boost::asio::ip::udp::endpoint & remote_endpoint
):
  _server_udp(server_udp),
  _remote_endpoint(remote_endpoint)
{
}

command_udp::~command_udp()
{
}

// command
void command_udp::send_response(const std::string & message)
{
  _server_udp->send_response(_remote_endpoint, message);
}
