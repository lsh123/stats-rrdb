/*
 ============================================================================
 Name        : stats-rrdb-tests.cpp
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

#include "tests/stats_rrdb_tests.h"
#include "tests/lru_tests.h"

int main(int argc, char ** argv)
{

  try {
    // init
    srand(time(NULL));

    //
    // lru_cache
    //
    TEST_START("lru_cache");
    lru_tests::run();
    TEST_END("lru_cache");



    // TODO
    // main server
    // boost::shared_ptr<server> main_server(new server());
    // main_server->initialize(cfg);
    // boost::shared_ptr<rrdb_test> test(new rrdb_test(main_server->_rrdb));
    // test->run(params);

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
