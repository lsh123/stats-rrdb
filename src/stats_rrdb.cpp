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
#include <time.h>
#include <signal.h>

#include <boost/shared_ptr.hpp>

#include "log.h"
#include "config.h"
#include "exception.h"
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
    boost::shared_ptr<server> main_server(new server(cfg));
    if(cfg->has("daemon")) {
        main_server->daemonize(cfg->get<std::string>("daemon"));
    }
    if(cfg->has("server.user")) {
        main_server->setuid_user(cfg->get<std::string>("server.user"));
    }
    main_server->run();

  } catch (std::exception & e) {
    std::cerr << "EXCEPTION: " << e.what() << std::endl;
    return(1);
  } catch (...) {
    std::cerr << "EXCEPTION: Unknown Exception" << std::endl;
    return(1);
  }

  return(0);
}
