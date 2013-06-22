/*
 * rrdb_test.cpp
 *
 *  Created on: Jun 17, 2013
 *      Author: aleksey
 */
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "rrdb/rrdb.h"

#include "server/thread_pool.h"

#include "tests/rrdb_test.h"

#include "common/log.h"
#include "common/exception.h"

#include "tests/stats_rrdb_tests.h"




rrdb_test::rrdb_test(const boost::shared_ptr<rrdb> & rrdb) :
  _rrdb(rrdb)
{
}

rrdb_test::~rrdb_test()
{
}

// params string format: <test_name>|<param1>|<param2>|....
rrdb_test::params_t rrdb_test::parse_params(const std::string & params_str)
{
  params_t res;
  boost::algorithm::split(res, params_str, boost::algorithm::is_any_of("|"), boost::algorithm::token_compress_off);
  return res;
}

void rrdb_test::cleanup(const my::size_t & metrics_num)
{

  std::string metric_name;
  for(my::size_t ii = 0; ii < metrics_num; ii++) {
      metric_name = rrdb_test::get_test_metric_name(ii);
      if(_rrdb->find_metric(metric_name)) {
          _rrdb->drop_metric(metric_name);
      }
  }
}

std::string rrdb_test::get_test_metric_name(const my::size_t & n)
{
  char buf[64];
  snprintf(buf, sizeof(buf), "test.metric.%lu", n);
  return buf;
}


void rrdb_test::run(const std::string & params_str) {
  // parse params string
  params_t params = rrdb_test::parse_params(params_str);
  if(params.size() < 1) {
      throw exception("Invalid test params: expected <test_name>|<param1>|<param2>|....");
  }

  if(params[0] == "perf") {
      this->run_perf_test(params);
  } else if(params[0] == "select") {
    this->run_select_test(params);
  } else if(params[0] == "lru_cache") {
      this->run_lru_cache_test(params);
  } else {
      throw exception("Invalid test '%s'", params[0].c_str());
  }
}

rrdb_test::t_csv_data rrdb_test::parse_csv_data(const t_memory_buffer_data & data)
{
  // break into lines
  std::vector<std::string> lines;
  boost::algorithm::split(lines, data, boost::algorithm::is_any_of("\n"), boost::algorithm::token_compress_off);

  // break fields in the line
  std::vector<std::string> v;
  rrdb_test::t_csv_data res;
  res.reserve(lines.size());
  BOOST_FOREACH(const std::string & line, lines) {
    if(line.empty()) continue;

    boost::algorithm::split(v, line, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_off);
    res.push_back(v);
    v.clear();
  };

  return res;
}

// SELECT test params:  <test_name>
void rrdb_test::run_select_test(const params_t & params)
{
  if(params.size() != 1) {
      throw exception("Invalid test params: expected <test_name>");
  }


  // Post-test cleanup
  std::cout << "Post-test clean up..." << std::endl;
  this->cleanup(1);
}

// No params
void rrdb_test::run_lru_cache_test(const rrdb_test::params_t & params)
{
  if(params.size() != 1) {
      throw exception("Invalid test params: expected <test_name>");
  }


  // done
  std::cout << "Finished LRU cache test" << std::endl;
}

// Perf test params:  <test_name>|<threads>|<tasks>|<metrics>
void rrdb_test::run_perf_test(const rrdb_test::params_t & params)
{
  if(params.size() != 4) {
      throw exception("Invalid test params: expected <test_name>|<threads>|<tasks>|<metrics>");
  }
  my::size_t threads_num = boost::lexical_cast<my::size_t>(params[1]);
  my::size_t tasks_num   = boost::lexical_cast<my::size_t>(params[2]);
  my::size_t metrics_num = boost::lexical_cast<my::size_t>(params[3]);


  // pre-test cleanup
  std::cout << "Pre-test clean up..." << std::endl;
  this->cleanup(metrics_num);

  // run test
  std::cout << "Starting performance test with "
    << threads_num << " threads, "
    << tasks_num << " tasks, "
    << metrics_num << " metrics"
    << std::endl;


  std::cout << "Finished performance test with "
        << tasks_num << " tasks, "
        << metrics_num << " metrics, "
        << threads_num << " threads in "
        << (ts2 - ts1) << " secs"
        << std::endl;
  std::cout
        << "Per task: "
        << (ts2 - ts1) / (double)tasks_num << " secs"
        << std::endl;

  // Post-test cleanup
  std::cout << "Post-test clean up..." << std::endl;
  this->cleanup(metrics_num);
}
