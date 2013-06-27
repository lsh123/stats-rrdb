/*
 * journal_file_tests.cpp
 *
 *  Created on: Jun 25, 2013
 *      Author: aleksey
 */

#include "parser/retention_policy.h"

#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_files_cache.h"
#include "rrdb/rrdb_journal_file.h"

#include "tests/stats_rrdb_tests.h"
#include "tests/journal_file_tests.h"
#include "rrdb/rrdb_metric_tuples_cache.h"

#define TEST_METRIC_NAME  "test.journal_file"

journal_file_tests::journal_file_tests()
{
}

journal_file_tests::~journal_file_tests()
{
}

void journal_file_tests::initialize(const std::string & path)
{
  _files_cache.reset(new rrdb_files_cache());
  _tuples_cache.reset(new rrdb_metric_tuples_cache(_files_cache));
  _journal_file.reset(new rrdb_journal_file(_files_cache));

  // create config
  t_test_config_data config_data;
  config_data["rrdb.path"]                  = path;
  config_data["rrdb.open_files_cache_size"] = "2";
  config_data["rrdb.open_files_cache_purge_threshold"] = "1.0"; // make sure we kick out at most one

  //
  boost::shared_ptr<config> cfg = test_setup_config(path, config_data);

  // initiliaze
  _files_cache->initialize(cfg);
  _journal_file->initialize();
}

void journal_file_tests::cleanup()
{
  _files_cache->delete_file(rrdb_metric::construct_filename(TEST_METRIC_NAME));
  _journal_file->delete_journal_file();
}

void journal_file_tests::test_apply_journal(const int & n)
{
  my::time_t ts = time(NULL);
  my::value_t last_value;
  my::time_t  last_value_ts;

  TEST_SUBTEST_START(n, "apply_journal", false);

  // create metric
  t_retention_policy policy = retention_policy_parse("1 sec for 1 hour, 30 sec for 1 day");
  boost::intrusive_ptr<rrdb_metric> metric(new rrdb_metric());
  metric->create(TEST_METRIC_NAME, policy);
  metric->save_file(_files_cache);

  // check
  last_value = 0;
  last_value_ts = 0;
  metric->get_last_value(last_value, last_value_ts);
  TEST_CHECK_EQUAL(last_value, 0);
  TEST_CHECK_EQUAL(last_value_ts, 0);

  // update
  metric->update(_tuples_cache, ts, 1.0);
  TEST_CHECK_EQUAL(metric->is_dirty(), true);

  // check
  last_value = 0;
  last_value_ts = 0;
  metric->get_last_value(last_value, last_value_ts);
  TEST_CHECK_EQUAL(last_value, 1);
  TEST_CHECK_EQUAL(last_value_ts, ts);

  // save
  my::size_t dbc = metric->save_dirty_blocks(_files_cache, _journal_file);
  TEST_CHECK_EQUAL(metric->is_dirty(), false);
  TEST_CHECK_EQUAL(*(metric->get_filename()), std::string(_journal_file->get_filename()));
  TEST_CHECK_EQUAL(dbc + 1, _journal_file->get_blocks_count()); // +1 for the metric's header

  // write data to the metric file
  _journal_file->apply_journal(metric->get_filename());

  // destroy
  metric.reset();

  // and then load back again
  my::filename_t filename = rrdb_metric::construct_filename(TEST_METRIC_NAME);
  metric.reset(new rrdb_metric(filename));
  metric->load_file(_files_cache);

  // check
  last_value = 0;
  last_value_ts = 0;
  metric->get_last_value(last_value, last_value_ts);
  TEST_CHECK_EQUAL(last_value, 1);
  TEST_CHECK_EQUAL(last_value_ts, ts);

  // done
  TEST_SUBTEST_END();
}

void journal_file_tests::test_save_and_load_journal(const int & n)
{
  my::time_t ts = time(NULL);
  my::value_t last_value;
  my::time_t  last_value_ts;

  TEST_SUBTEST_START(n, "save_and_load_journal", false);

  // create metric
  t_retention_policy policy = retention_policy_parse("1 sec for 1 hour, 30 sec for 1 day");
  boost::intrusive_ptr<rrdb_metric> metric(new rrdb_metric());
  metric->create(TEST_METRIC_NAME, policy);
  metric->save_file(_files_cache);

  // check
  last_value = 0;
  last_value_ts = 0;
  metric->get_last_value(last_value, last_value_ts);
  TEST_CHECK_EQUAL(last_value, 0);
  TEST_CHECK_EQUAL(last_value_ts, 0);

  // update
  metric->update(_tuples_cache, ts, 1.0);
  TEST_CHECK_EQUAL(metric->is_dirty(), true);

  // check
  last_value = 0;
  last_value_ts = 0;
  metric->get_last_value(last_value, last_value_ts);
  TEST_CHECK_EQUAL(last_value, 1);
  TEST_CHECK_EQUAL(last_value_ts, ts);

  // save
  my::size_t dbc = metric->save_dirty_blocks(_files_cache, _journal_file);
  TEST_CHECK_EQUAL(metric->is_dirty(), false);
  TEST_CHECK_EQUAL(*(metric->get_filename()), std::string(_journal_file->get_filename()));
  TEST_CHECK_EQUAL(dbc + 1, _journal_file->get_blocks_count()); // +1 for the metric's header

  // write data to the journal file
  _journal_file->save_journal_file();
  _journal_file->clear();

  // and then load back again
  TEST_CHECK_EQUAL(_journal_file->is_journal_file_present(), true);
  _journal_file->load_journal_file();
  TEST_CHECK(strcmp(metric->get_filename()->c_str(), _journal_file->get_filename()) == 0);
  TEST_CHECK_EQUAL(dbc + 1, _journal_file->get_blocks_count()); // +1 for the metric's header

  // write data to the metric file
  my::filename_t filename(new std::string(_journal_file->get_filename()));
  _journal_file->apply_journal(filename);

  // destroy and reload metric
  metric.reset();
  metric.reset(new rrdb_metric(filename));
  metric->load_file(_files_cache);

  // check
  last_value = 0;
  last_value_ts = 0;
  metric->get_last_value(last_value, last_value_ts);
  TEST_CHECK_EQUAL(last_value, 1);
  TEST_CHECK_EQUAL(last_value_ts, ts);

  // done
  TEST_SUBTEST_END();
}

void journal_file_tests::run(const std::string & path)
{
  // setup
  journal_file_tests test;

  // tests
  test.initialize(path);
  test.cleanup();
  test.test_apply_journal(0);

  test.initialize(path);
  test.cleanup();
  test.test_save_and_load_journal(1);

  // cleanup
  test.cleanup();
}
