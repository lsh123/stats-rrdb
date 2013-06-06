/*
 * server.cpp
 *
 *  Created on: Jun 2, 2013
 *      Author: aleksey
 */
#include "server.h"

#include "log.h"
#include "config.h"
#include "thread_pool.h"

#include "rrdb.h"
#include "server_udp.h"
#include "server_tcp.h"

server::server(boost::shared_ptr<config> config)
{
  log::write(log::LEVEL_INFO, "Starting the server");

  // init
  _rrdb.reset(new rrdb(config));
  _server_udp.reset(new server_udp(_io_service, _rrdb, config));
  _server_tcp.reset(new server_tcp(_io_service, _rrdb, config));

  log::write(log::LEVEL_INFO, "Started the server");
}

server::~server()
{
}

void server::run()
{
  _server_udp->start_receive();
  _server_tcp->start_accept();

  _io_service.run();
}
