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
#include "server_udp.h"

server::server(boost::shared_ptr<config> config) :
  _config(config)
{
  // init
  _thread_pool.reset(new thread_pool(config));
  _server_udp.reset(new server_udp(_thread_pool, _io_service, config));
}

server::~server()
{
}

void server::run()
{
  _io_service.run();
}
