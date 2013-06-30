/*
 * query_tests.cpp
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */

#include <boost/foreach.hpp>

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
  TEST_SUBTEST_START(n, "create()", false);

  boost::intrusive_ptr<rrdb_metric> metric;

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
  TEST_SUBTEST_START(n, "select ALL data", false);

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
  TEST_SUBTEST_START(n, "select last 5 sec", false);

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

void query_tests::test_select_all_group_by(const int & n, const my::size_t & group_by, const std::string & msg)
{
  TEST_SUBTEST_START(n, msg, false);

  char buf[1024];
  snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu group by %lu secs; ",
       _metric_name.c_str(),
       _start_ts,
       _end_ts,
       group_by
   );

  // query
  t_memory_buffer_data res_data;
  t_memory_buffer res(res_data);
  _rrdb->execute_query_statement(buf, res);

  t_test_csv_data parsed_data;
  test_parse_csv_data(res_data, parsed_data);

  // count: 1 header row + "group_by" sec rows
  my::size_t data_rows_count = _count / group_by;
  if(_count % group_by > 0) {
      ++data_rows_count;
  }
  TEST_CHECK_EQUAL(parsed_data.size(), (1 + data_rows_count));

  // all should be "group_by" sec rows with "group_by" data points each
  // except the first row which is weird
  my::time_t ts = _start_ts;
  for(my::size_t ii = parsed_data.size() - 1; ii > 0; --ii, ts += group_by) {
      const std::vector<std::string> & row(parsed_data[ii]);

      // the first data row is weird
      my::size_t row_count = group_by;
      if(ii == 1 && _count % group_by > 0) {
          row_count = _count % group_by;
      }
      // std::cerr << "ROW: " << ii << " TS: " << ts << " expected: " << row_count << std::endl;

      TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(row[0]),  ts); // ts
      TEST_CHECK_EQUAL(boost::lexical_cast<my::value_t>(row[1]), row_count);  // count = "group_by"
      TEST_CHECK_EQUAL(boost::lexical_cast<my::value_t>(row[2]), row_count);  // sum = "group_by"
      TEST_CHECK_EQUAL(boost::lexical_cast<my::value_t>(row[3]), 1);  // avg = 1
      TEST_CHECK_EQUAL(boost::lexical_cast<my::value_t>(row[4]), 0);  // stdev = 0
      TEST_CHECK_EQUAL(boost::lexical_cast<my::value_t>(row[5]), 1);  // min = 1
      TEST_CHECK_EQUAL(boost::lexical_cast<my::value_t>(row[6]), 1);  // max = 1
  }

  // done
  TEST_SUBTEST_END();
  TEST_DATA(buf, std::string(res_data.begin(), res_data.end()));
}

void query_tests::partial_interval_test(const int & n)
{
  TEST_SUBTEST_START(n, "partial_interval_test", false);

  // this is an interesting one... here we have second retention policy
  // that doesn't align with the first tuples (i.e. first tuples cover
  // only half of it). as the result we don't know where these first
  // tuples are actually located on the block interval and we just
  // assume equal distribution in the select() and fail

  boost::intrusive_ptr<rrdb_metric> metric;

  // check it doesn't exits
  metric = _rrdb->find_metric(_metric_name);
  TEST_CHECK_EQUAL(metric.get(), NULL);

  // create
  std::string orig_policy = "1 sec for 10 secs, 10 secs for 30 secs";
  _rrdb->create_metric(_metric_name, retention_policy_parse(orig_policy));

  // check it exists
  metric = _rrdb->find_metric(_metric_name);
  TEST_CHECK_NOT_EQUAL(metric.get(), NULL);

  // insert data
  my::time_t start_ts = 30000000000 + 5; // this is 5 secs off of the 10 secs (second freq)
  my::time_t end_ts   = start_ts + 15;
  for(my::time_t ts = start_ts; ts < end_ts; ts += 1) {
      _rrdb->update_metric(_metric_name, ts, 1.0);
  }

  // now select all group by 1 year
  char buf[1024];
  snprintf(buf, sizeof(buf),  "select * from '%s' between %lu and %lu group by 1 year; ",
       _metric_name.c_str(),
       start_ts,
       end_ts
   );
  t_memory_buffer_data res_data;
  t_memory_buffer res(res_data);
  _rrdb->execute_query_statement(buf, res);

  std::cerr << "TODO: " << std::string(res_data.begin(), res_data.end()) << std::endl;

  t_test_csv_data parsed_data;
  test_parse_csv_data(res_data, parsed_data);
  TEST_CHECK_EQUAL(parsed_data.size(), (1 + 1)); // 1 header row and 1 "group by 1 year"

  // check the data row
  const std::vector<std::string> & row(parsed_data[1]);
  TEST_CHECK_EQUAL(boost::lexical_cast<my::time_t>(row[0]),  start_ts);
  // this is the key check: we "shifted" the start ts but we should have aligned
  // blocks by the first block so we get back exactly the count we put in
  // w/o any partial interval adjustments
  TEST_CHECK_EQUAL(boost::lexical_cast<my::value_t>(row[1]), end_ts - start_ts);

  // done
  TEST_SUBTEST_END();
}

/*
void query_tests::test_template(const int & n)
{
  TEST_SUBTEST_START(n, "select 5_sec data", false);


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
  int ii = 0;

  // the following tests use the metric from create()
  test.test_create(ii++);
  test.test_select_all(ii++);
  test.test_select_5_sec(ii++);
  test.test_select_all_group_by(ii++, 1,  "select all group by 1 sec");
  test.test_select_all_group_by(ii++, 2,  "select all group by 2 secs");
  test.test_select_all_group_by(ii++, 3,  "select all group by 3 secs");
  test.test_select_all_group_by(ii++, 13, "select all group by 13 secs");
  test.test_select_all_group_by(ii++, 30, "select all group by 30 secs");
  test.test_select_all_group_by(ii++, 67, "select all group by 67 secs");
  test.test_select_all_group_by(ii++, 24*60*60, "select all group by 1 day");
  test.test_select_all_group_by(ii++, 365*24*60*60, "select all group by 1 year");

  // need new metric
  test.cleanup();
  test.partial_interval_test(ii++);

  // cleanup
  test.cleanup();
}

