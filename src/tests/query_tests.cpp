/*
 * query_tests.cpp
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric.h"

#include "common/log.h"
#include "common/config.h"
#include "common/exception.h"

#include "tests/query_tests.h"
#include "tests/stats_rrdb_tests.h"

query_tests::query_tests() :
  _rrdb(new rrdb()),
  _metric_name("test.metric"),
  _start_ts(1371600000),     // Wed, 19 Jun 2013 00:00:00 GMT
  _end_ts(1371600000 + 300), // 5 mins of data
  _freq(1),                  // 1 sec updates
  _count(0)
{
}

query_tests::~query_tests()
{
}

void query_tests::initialize(const std::string & path)
{
  // create config
  t_test_config_data config_data;
  config_data["rrdb.path"] = path;
  config_data["log.level"] = "debug3";
  config_data["log.destination"] = path + "./query_tests.log";

  //
  boost::shared_ptr<config> cfg = test_setup_config(path, config_data);


  // initiliaze
  _rrdb->initialize(cfg);
}

void query_tests::cleanup()
{
  // ensure metric doesn't exist from previous run
  if(_rrdb->find_metric(_metric_name)) {
      _rrdb->drop_metric(_metric_name);
  }
}

void query_tests::test_create(const int & n)
{
  TEST_SUBTEST_START(n, "create()");

  boost::shared_ptr<rrdb_metric> metric;

  // check it doesn't exits
  metric = _rrdb->find_metric(_metric_name);
  TEST_CHECK_EQUAL(metric.get(), NULL);

  // create
  std::string orig_policy = "1 sec for 10 secs, 10 secs for 30 secs, 30 secs for 1 hour";
  _rrdb->create_metric(_metric_name, retention_policy_parse(orig_policy));

  // check it exists
  metric = _rrdb->find_metric(_metric_name);
  TEST_CHECK_NOT_EQUAL(metric.get(), NULL);

  // check policy
  std::string metric_policy = retention_policy_write(metric->get_policy());
  TEST_CHECK_EQUAL(orig_policy, metric_policy);

  // insert data
  for(my::time_t ts = _start_ts; ts < _end_ts; ts += _freq, ++_count) {
      _rrdb->update_metric(_metric_name, ts, 1.0);
  }

  // done
  TEST_SUBTEST_END();
}

void query_tests::test_select_all(const int & n)
{
  TEST_SUBTEST_START(n, "select ALL data");

  char buf[1024];
  snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu ; ",
      _metric_name.c_str(),
      _start_ts,
      _start_ts + _end_ts
  );

  t_memory_buffer_data res_data;
  t_memory_buffer res(res_data);
  _rrdb->execute_query_statement(buf, res);

  t_test_csv_data parsed_data;
  test_parse_csv_data(res_data, parsed_data);

  // count: 1 header row + 10 rows 1 sec each + (30 / 10) rows 10 sec each + 30 sec rows
  TEST_CHECK_EQUAL(parsed_data.size(), (1 + 10 / 1 + (30 / 10 - 1) + (_count / 30 - 1) ) );

  // first line is the latest: latest ts + count = 1
  TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data[1][0]), (my::time_t)(_end_ts - _freq));
  TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data[1][1]), 1);

  // last line is the oldest: oldest ts + full 30 sec interval data
  TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data.back()[0]), _start_ts);
  TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data.back()[1]), 30);

  // done
  TEST_SUBTEST_END();
  TEST_DATA(buf, std::string(res_data.begin(), res_data.end()));
}

void query_tests::test_select_5_sec(const int & n)
{
  TEST_SUBTEST_START(n, "select last 5 sec");

  char buf[1024];
  snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu ; ",
      _metric_name.c_str(),
      _end_ts - 5 * _freq,
      _end_ts
  );

  t_memory_buffer_data res_data;
  t_memory_buffer res(res_data);
  _rrdb->execute_query_statement(buf, res);

  t_test_csv_data parsed_data;
  test_parse_csv_data(res_data, parsed_data);

  // expect 5 rows + header
  TEST_CHECK_EQUAL(parsed_data.size(), (1 + 5) );

  // first line is the latest: latest ts + count = 1
  TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data[1][0]), (my::time_t)(_end_ts - _freq));
  TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data[1][1]), 1);

  // last line is the oldest: (latest ts - 5 secs) + count = 1
  TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data.back()[0]), (my::time_t)(_end_ts - 5 * _freq));
  TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data.back()[1]), 1);

  // done
  TEST_SUBTEST_END();
  TEST_DATA(buf, std::string(res_data.begin(), res_data.end()));
}

void query_tests::test_select_all_group_by_30_sec(const int & n)
{
  TEST_SUBTEST_START(n, "select all group by 30 sec");

  char buf[1024];
  snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu group by 30 secs; ",
      _metric_name.c_str(),
      _start_ts,
      _end_ts
  );

  t_memory_buffer_data res_data;
  t_memory_buffer res(res_data);
  _rrdb->execute_query_statement(buf, res);

  t_test_csv_data parsed_data;
  test_parse_csv_data(res_data, parsed_data);

  // count: 1 header row + 30 sec rows
  TEST_CHECK_EQUAL(parsed_data.size(), (1 + _count / 30) );

  // first line is the latest: latest ts + count = 1
  TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data[1][0]), (my::time_t)(_end_ts - 30));
  TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data[1][1]), 30);

  // last line is the oldest: oldest ts + full 30 sec interval data
  TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data.back()[0]), _start_ts);
  TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data.back()[1]), 30);

  // done
  TEST_SUBTEST_END();
  TEST_DATA(buf, std::string(res_data.begin(), res_data.end()));
}

void query_tests::test_select_all_group_by_1_year(const int & n)
{
  TEST_SUBTEST_START(n, "select all group by 1 year");

  char buf[1024];
  snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu group by 1 year; ",
      _metric_name.c_str(),
      _start_ts,
      _end_ts
  );

  t_memory_buffer_data res_data;
  t_memory_buffer res(res_data);
  _rrdb->execute_query_statement(buf, res);

  t_test_csv_data parsed_data;
  test_parse_csv_data(res_data, parsed_data);

  // count: 1 header row + 1 data row
  TEST_CHECK_EQUAL(parsed_data.size(), 2);

  // first and only line: start ts, count
  TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(parsed_data[1][0]), _start_ts);
  TEST_CHECK_EQUAL(boost::lexical_cast<my::size_t>(parsed_data[1][1]), _count);

  // done
  TEST_SUBTEST_END();
  TEST_DATA(buf, std::string(res_data.begin(), res_data.end()));
}

/*
void query_tests::test_template(const int & n)
{
  TEST_SUBTEST_START(n, "select 5_sec data");


  // done
  TEST_SUBTEST_END();
  TEST_DATA(buf, std::string(res_data.begin(), res_data.end()));
}
*/


void query_tests::run(const std::string & path)
{
  // setup
  query_tests test;
  test.initialize(path);
  test.cleanup();

  // tests
  test.test_create(0);
  test.test_select_all(1);
  test.test_select_5_sec(2);
  test.test_select_all_group_by_30_sec(3);
  test.test_select_all_group_by_1_year(4);

  // cleanup
  test.cleanup();
}

