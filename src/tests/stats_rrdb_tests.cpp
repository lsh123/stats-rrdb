/*
 ============================================================================
 Name        : stats-rrdb-tests.cpp
 Author      : Aleksey Sanin
 Version     :
 Copyright   : Copyright 2013, Aleksey Sanin. All Rights Reserved.
 ============================================================================
 */

#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <signal.h>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "common/log.h"
#include "common/config.h"
#include "common/exception.h"
#include "server/server.h"

#include "tests/stats_rrdb_tests.h"

#include "tests/lru_tests.h"
#include "tests/files_cache_tests.h"
#include "tests/journal_file_tests.h"
#include "tests/query_tests.h"
#include "tests/update_tests.h"
#include "tests/parsers_tests.h"

boost::shared_ptr<config> test_setup_config(const std::string & path, const t_test_config_data & data)
{
  // ensure top folder exists
  boost::filesystem::create_directories(path);

  // create config
  std::string config_path = path + "./stats_rrdb_test.conf";
  std::fstream fs(config_path.c_str(), std::ios_base::out | std::ios_base::trunc);
  fs.exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs
  BOOST_FOREACH(const t_test_config_data::value_type & v, data) {
    fs << v.first << "=" << v.second << std::endl;
  }
  fs.flush();
  fs.close();

  // load config
  const char * argv[] = { "stats-rrdb-test", "-c", config_path.c_str() };
  boost::shared_ptr<config> cfg(new config());
  cfg->init(sizeof(argv) / sizeof(argv[0]), argv);

  // done
  return cfg;
}


void test_parse_csv_data(const t_memory_buffer_data & data, t_test_csv_data & res)
{
  // break into lines
  std::vector<std::string> lines;
  boost::algorithm::split(lines, data, boost::algorithm::is_any_of("\n"), boost::algorithm::token_compress_off);

  // break fields in the line
  std::vector<std::string> v;
  res.reserve(lines.size());
  BOOST_FOREACH(const std::string & line, lines) {
    if(line.empty()) continue;

    boost::algorithm::split(v, line, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_off);
    res.push_back(v);
    v.clear();
  };
}


int main(int argc, char ** argv)
{
  std::string path = argc > 1 ? argv[1] : "/tmp/stats_rrdb_tests.tmp/";

  try {
    // init
    srand(time(NULL));

    //
    // setup log
    //
    t_test_config_data config_data;
    config_data["log.level"] = "info";
    config_data["log.destination"] = path + "./unit_tests.log";
    boost::shared_ptr<config> cfg = test_setup_config(path, config_data);
    log::init(*cfg);

    //
    // lru_cache
    //
    TEST_START("lru_cache");
    lru_tests::run();
    TEST_END("lru_cache");

    //
    // files_cache_tests
    //
    TEST_START("files_cache_tests");
    files_cache_tests::run(path);
    TEST_END("files_cache_tests");

    //
    // journal_file_tests
    //
    TEST_START("journal_file_tests");
    journal_file_tests::run(path);
    TEST_END("journal_file_tests");

    //
    // parsers_tests
    //
    TEST_START("parsers_tests");
    parsers_tests::run();
    TEST_END("parsers_tests");

    //
    // query_tests
    //
    TEST_START("query_tests");
    query_tests::run(path);
    TEST_END("query_tests");

    //
    // update_tests
    //
    TEST_START("update_tests");
    update_tests::run(path);
    TEST_END("update_tests");


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
