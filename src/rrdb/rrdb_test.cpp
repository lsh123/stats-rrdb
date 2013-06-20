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

#include "rrdb/rrdb_test.h"
#include "rrdb/rrdb.h"

#include "server/thread_pool.h"

#include "exception.h"

#define TEST_CHECK_EQUAL( a, b ) \
    if( (a) != (b) ) { \
        std::cerr << "TEST CHECK EQUAL FAILURE: '" \
          << (#a) << "' = " << (a) << " and '" << (#b) << "' = " << (b) \
          << std::endl; \
    }

class rrdb_test_perf_task :
    public thread_pool_task
{
public:
  rrdb_test_perf_task(boost::shared_ptr<rrdb> rrdb, const std::string & cmd) :
    _rrdb(rrdb),
    _cmd(cmd)
  {
  }
  virtual ~rrdb_test_perf_task()
  {
  }

public:
  // thread_pool_task
  void run()
  {
      memory_buffer_data_t output_buffer;
      memory_buffer_t res(output_buffer);

      _rrdb->execute_udp_command(_cmd, res);
      res.flush();
  }

private:
  boost::shared_ptr<rrdb> _rrdb;
  std::string             _cmd;
}; // rrdb_test_perf_task



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
} else {
      throw exception("Invalid test '%s'", params[0].c_str());
  }
}

rrdb_test::csv_data_t rrdb_test::parse_csv_data(const memory_buffer_data_t & data)
{
  // break into lines
  std::vector<std::string> lines;
  boost::algorithm::split(lines, data, boost::algorithm::is_any_of("\n"), boost::algorithm::token_compress_off);

  // break fields in the line
  std::vector<std::string> v;
  rrdb_test::csv_data_t res;
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

  // pre-test cleanup
  std::cout << "Pre-test clean up..." << std::endl;
  this->cleanup(1);

  // setup
  std::cout << "Setup..." << std::endl;
  std::string metric_name = rrdb_test::get_test_metric_name(1);
  my::time_t start_ts = 1371600000; // Wed, 19 Jun 2013 00:00:00 GMT
  my::size_t tests_count = 180;
  _rrdb->create_metric(metric_name, retention_policy_parse("1 sec FOR 10 secs, 10 secs for 30 secs, 30 sec for 1 hour"));

  // insert data
  std::cout << "Insert data..." << std::endl;
  for(my::size_t ts = 0; ts < tests_count; ++ts) {
      _rrdb->update_metric(metric_name, start_ts + ts, 1.0);
  }

  //
  // TEST 1: get all data
  //
  std::cout << "=== QUERY ALL DATA" << std::endl;
  {
    char buf[1024];
    snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu ; ",
        metric_name.c_str(),
        start_ts,
        start_ts + tests_count + 100
    );

    memory_buffer_data_t res_data;
    memory_buffer_t res(res_data);
    _rrdb->execute_tcp_command(buf, res);

    rrdb_test::csv_data_t parsed_data = rrdb_test::parse_csv_data(res_data);

    // count: 1 header row + 10 rows 1 sec each + (30 / 10) rows 10 sec each + 30 sec rows
    TEST_CHECK_EQUAL(parsed_data.size(), (1 + 10 / 1 + (30 / 10 - 1) + (tests_count / 30 - 1) ) );

    // first line is the latest: latest ts + count = 1
    TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data[1][0]), (my::time_t)(start_ts + tests_count - 1));
    TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data[1][1]), 1);

    // last line is the oldest: oldest ts + full 30 sec interval data
    TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data.back()[0]), start_ts);
    TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data.back()[1]), 30);

    // test 1: done
    std::cout << "=== Result for " << buf << std::endl;
    std::cout << std::string(res_data.begin(), res_data.end()) << std::endl;
  }

  //
  // TEST 2: get 5 secs
  //
  std::cout << "=== QUERY LAST 5 SEC" << std::endl;
  {
    char buf[1024];
    snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu ; ",
        metric_name.c_str(),
        start_ts + tests_count - 5,
        start_ts + tests_count
    );

    memory_buffer_data_t res_data;
    memory_buffer_t res(res_data);
    _rrdb->execute_tcp_command(buf, res);

    rrdb_test::csv_data_t parsed_data  = rrdb_test::parse_csv_data(res_data);

    // expect 5 rows + header
    TEST_CHECK_EQUAL(parsed_data.size(), (1 + 5) );

    // first line is the latest: latest ts + count = 1
    TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data[1][0]), (my::time_t)(start_ts + tests_count - 1));
    TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data[1][1]), 1);

    // last line is the oldest: (latest ts - 5 secs) + count = 1
    TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data.back()[0]), (my::time_t)(start_ts + tests_count - 5));
    TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data.back()[1]), 1);

    // test 2: done
    std::cout << "=== Result for " << buf << std::endl;
    std::cout << std::string(res_data.begin(), res_data.end()) << std::endl;
  }

  //
  // TEST 3: get all data group by 30 secs (end ts shift is 30)
  //
  std::cout << "=== QUERY ALL DATA GROUP BY 30 secs" << std::endl;
  {
    char buf[1024];
    snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu group by 30 secs; ",
        metric_name.c_str(),
        start_ts,
        start_ts + tests_count + 30
    );

    memory_buffer_data_t res_data;
    memory_buffer_t res(res_data);
    _rrdb->execute_tcp_command(buf, res);

    rrdb_test::csv_data_t parsed_data = rrdb_test::parse_csv_data(res_data);

    // count: 1 header row + 30 sec rows
    TEST_CHECK_EQUAL(parsed_data.size(), (1 + tests_count / 30) );

    // first line is the latest: latest ts + count = 1
    TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data[1][0]), (my::time_t)(start_ts + tests_count - 30));
    TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data[1][1]), 30);

    // last line is the oldest: oldest ts + full 30 sec interval data
    TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data.back()[0]), start_ts);
    TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data.back()[1]), 30);

    // test 1: done
    std::cout << "=== Result for " << buf << std::endl;
    std::cout << std::string(res_data.begin(), res_data.end()) << std::endl;
  }


  //
  // TEST 4: get all data group by 30 secs (end ts shift is 37)
  //
  std::cout << "=== QUERY ALL DATA GROUP BY 30 secs (shift 37 secs)" << std::endl;
  {
    char buf[1024];
    snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu group by 30 secs; ",
        metric_name.c_str(),
        start_ts,
        start_ts + tests_count + 37
    );

    memory_buffer_data_t res_data;
    memory_buffer_t res(res_data);
    _rrdb->execute_tcp_command(buf, res);

    rrdb_test::csv_data_t parsed_data = rrdb_test::parse_csv_data(res_data);

    // count: 1 header row + 30 sec rows
    TEST_CHECK_EQUAL(parsed_data.size(), (1 + tests_count / 30) );

    // first line is the latest: latest ts + count = 1
    TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data[2][1]), 30);

    // last line is the oldest: oldest ts + full 30 sec interval data
    TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data.back()[1]), 30);

    // test 1: done
    std::cout << "=== Result for " << buf << std::endl;
    std::cout << std::string(res_data.begin(), res_data.end()) << std::endl;
  }


  // Post-test cleanup
  std::cout << "Post-test clean up..." << std::endl;
  this->cleanup(1);
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
  boost::shared_ptr<thread_pool> threads(new thread_pool(threads_num));
  time_t ts1(time(NULL));
  char buf[1024];
  for(my::size_t ii = 0; ii < tasks_num; ii++) {
      snprintf(buf, sizeof(buf),  "u|%s|1|%lu",
          rrdb_test::get_test_metric_name(rand() % metrics_num).c_str(),
          ts1 + ii
      );

      boost::shared_ptr<rrdb_test_perf_task> task(new rrdb_test_perf_task(_rrdb, buf));
      threads->run(task);

      // wait if load factor is bigger than 2.0
      while(threads->get_load_factor() > 2.0) {
          LOG(log::LEVEL_DEBUG, "Load factor > 2.0, sleeping\n");
          boost::this_thread::sleep(boost::posix_time::milliseconds(100));
      }
  }

  // wait until done
  while(threads->get_load_factor() > 0) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  }

  // done - print results
  time_t ts2(time(NULL));

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
