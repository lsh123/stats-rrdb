/*
 ============================================================================
 Name        : stats-rrdb.cpp
 Author      : Aleksey Sanin
 Version     :
 Copyright   : Copyright 2013, Aleksey Sanin. All Rights Reserved.
 Description : Hello World in C++,
 ============================================================================
 */

#include <iostream>
#include <string>

#include <boost/shared_ptr.hpp>

#include "log.h"
#include "config.h"
#include "server.h"



int main(int argc, char ** argv)
{
  try
  {
    // load config
    boost::shared_ptr<config> cfg(new config());
    if(!cfg->init(argc, argv)) {
        return (0);
    }

    boost::asio::io_service io_service;
    udp_server server(io_service);
    io_service.run();
  } catch (std::exception& e) {
    log::write(log::LEVEL_ERROR, "%s", e.what());
    return(1);
  }

  return(0);
}
