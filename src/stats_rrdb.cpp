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

#include <boost/shared_ptr.hpp>

#include "log.h"
#include "config.h"
#include "exception.h"
#include "server.h"

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

    boost::shared_ptr<server> main_server(new server(cfg));
    main_server->run();
  } catch (std::exception & e) {
    log::write(log::LEVEL_ERROR, "%s", e.what());
    return(1);
  } catch (...) {
    log::write(log::LEVEL_ERROR, "Unknown un-handled exception");
    return(1);
  }

  return(0);
}
