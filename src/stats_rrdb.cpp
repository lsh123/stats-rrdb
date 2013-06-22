/*
 ============================================================================
 Name        : stats-rrdb.cpp
 Author      : Aleksey Sanin
 Version     :
 Copyright   : Copyright 2013, Aleksey Sanin. All Rights Reserved.
 ============================================================================
 */

#include <iostream>
#include <string>
#include <time.h>
#include <signal.h>

#include <boost/shared_ptr.hpp>

#include "common/log.h"
#include "common/config.h"
#include "common/exception.h"
#include "server/server.h"

int main(int argc, char ** argv)
{

  try {
    // init
    srand(time(NULL));

    // load config
    boost::shared_ptr<config> cfg(new config());
    if(!cfg->init(argc, argv)) {
        return (0);
    }


    // main server
    boost::shared_ptr<server> main_server(new server());
    main_server->initialize(cfg);

    if(cfg->has("daemon")) {
        main_server->daemonize(cfg->get<std::string>("daemon"));
    }
    if(cfg->has("server.user")) {
        main_server->setuid_user(cfg->get<std::string>("server.user"));
    }
    main_server->run();

  } catch (const std::exception & e) {
    LOG(log::LEVEL_CRITICAL,  "EXCEPTION: %s", e.what());

    std::cerr << "EXCEPTION: " << e.what() << std::endl;
    return(1);
  } catch (...) {
    LOG(log::LEVEL_CRITICAL,  "EXCEPTION: Unknown exception");

    std::cerr << "EXCEPTION: Unknown Exception" << std::endl;
    throw;
  }

  return(0);
}
