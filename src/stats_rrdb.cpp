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
#include "server.h"


// main server
boost::shared_ptr<server> main_server;

// Handling signals
static void signal_handler(int signum)
{
  switch(signum) {
  case SIGHUP:
  case SIGINT:
  case SIGQUIT:
  case SIGABRT:
  case SIGTERM:
    log::write(log::LEVEL_INFO, "Received signal %d, exiting", signum);
    if(main_server) {
        main_server->stop();
        main_server.reset();
    }
    log::write(log::LEVEL_INFO, "Received signal %d, done", signum);

    exit(signum);
    break;
  default:
    log::write(log::LEVEL_ERROR, "Unexpected signal %d", signum);
    break;
  }
}

static void setup_signal_handler()
{
  signal(SIGHUP,  signal_handler);
  signal(SIGINT,  signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGTERM, signal_handler);
}

int main(int argc, char ** argv)
{

  try {
    // init
    srand(time(NULL));
    setup_signal_handler();

    // load config
    boost::shared_ptr<config> cfg(new config());
    if(!cfg->init(argc, argv)) {
        return (0);
    }

    main_server.reset(new server(cfg));
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
