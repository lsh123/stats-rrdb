/*
 * update_tests.cpp
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */
#include "tests/update_tests.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_journal_file.h"
#include "rrdb/rrdb_files_cache.h"
#include "rrdb/rrdb_metric_tuples_cache.h"
#include "rrdb/rrdb_metric.h"

#include "common/thread_pool.h"

#include "common/log.h"
#include "common/config.h"
#include "common/exception.h"

#include "tests/stats_rrdb_tests.h"

#define METRIC_NAME_TEMPLATE "test.metric.%lu"


class load_test_task :
    public thread_pool_task
{
public:
  load_test_task(boost::shared_ptr<rrdb> rrdb, const std::string & cmd) :
    _rrdb(rrdb),
    _cmd(cmd)
  {
  }
  virtual ~load_test_task()
  {
  }

public:
  // thread_pool_task
  void run()
  {
      t_memory_buffer_data output_buffer;
      t_memory_buffer res(output_buffer);

      _rrdb->execute_update_statement(_cmd, res);
      res.flush();
  }

private:
  boost::shared_ptr<rrdb> _rrdb;
  std::string             _cmd;
}; // load_test_task

update_tests::update_tests() :
  _rrdb(new rrdb())
{
}

update_tests::~update_tests()
{
}


void update_tests::initialize(const std::string & path)
{
  // create config
  t_test_config_data config_data;
  config_data["rrdb.path"] = path;
  boost::shared_ptr<config> cfg = test_setup_config(path, config_data);

  // initiliaze
  _rrdb->initialize(cfg);
}

void update_tests::cleanup(const my::size_t & num_metrics)
{
  for(my::size_t ii = 0; ii < num_metrics; ++ii) {
    char buf[1024];
    snprintf(buf, sizeof(buf), METRIC_NAME_TEMPLATE, ii);

    // ensure metric doesn't exist from previous run
    if(_rrdb->find_metric(buf)) {
        _rrdb->drop_metric(buf);
    }
  }
}

void update_tests::load_test(const int & n,
    const my::size_t & num_metrics,
    const my::size_t & num_threads,
    const my::size_t & num_tasks
) {
  char buf[1024];

  //
  snprintf(buf, sizeof(buf), "load test - %lu metrics, %lu threads, %lu tasks", num_metrics, num_threads, num_tasks);
  TEST_SUBTEST_START(n, buf, false);

  // cleanup locally
  this->cleanup(num_metrics);

  my::time_t ts(time(NULL));
  boost::shared_ptr<thread_pool> threads(new thread_pool(num_threads));
  boost::posix_time::ptime ts1 = boost::posix_time::microsec_clock::local_time();
  for(my::size_t ii = 0; ii < num_tasks; ii++) {
      snprintf(buf, sizeof(buf),
          "u|" METRIC_NAME_TEMPLATE "|1|%lu",
          rand() % num_metrics,
          ts + ii
      );

      boost::intrusive_ptr<load_test_task> task(new load_test_task(_rrdb, buf));
      threads->run(task);

      if(ii % 10000 == 0) {
          std::cout << "Processed " << ii << " tasks ..." << std::endl;
      }
  }
  std::cout << "Waiting for all tasks to complete" << std::endl;

  // wait until done
  while(threads->get_load_factor() > 0) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  }

  TEST_CHECK_EQUAL(threads->get_load_factor(), 0);

  // done - print results
  boost::posix_time::ptime ts2 = boost::posix_time::microsec_clock::local_time();
  boost::posix_time::time_duration delta = ts2 - ts1;

  snprintf(buf, sizeof(buf),  "%0.2f tasks per second",
      (double)num_tasks * boost::posix_time::seconds(1).total_microseconds() / (double)delta.total_microseconds()
  );

  // done
  TEST_SUBTEST_END2(buf);

  // cleanup locally
  this->cleanup(num_metrics);
}

void update_tests::last_block_test(const int & n)
{
  TEST_SUBTEST_START(n, "Test last block", false);
  my::time_t ts = time(NULL);

  // important we start clean
  this->cleanup(1);

  // create and save metric
  char buf[1024];
  snprintf(buf, sizeof(buf), METRIC_NAME_TEMPLATE, (my::size_t)0);
  t_retention_policy policy = retention_policy_parse("1 sec for 5 secs, 5 secs for 1 min");
  boost::intrusive_ptr<rrdb_metric> metric(new rrdb_metric());
  metric->create(buf, policy);
  my::filename_t filename = metric->get_filename();
  std::string fullpath = _rrdb->get_files_cache()->get_full_path(filename);

  // save and reset
  metric->save_file(_rrdb->get_files_cache());
  metric.reset();

  // ok, write some tuples
  for(my::time_t ii = 0; ii < (1 * INTERVAL_MIN) / (5 * INTERVAL_SEC) + 3; ++ii) {
      std::cout << "Iteration " << ii << ": file size " << boost::filesystem::file_size( fullpath ) << " bytes" << std::endl;

      // load metric
      metric.reset(new rrdb_metric(filename));
      metric->load_file(_rrdb->get_files_cache());

      // update
      metric->update(_rrdb->get_tuples_cache(), ts + ii * (5 * INTERVAL_SEC), 1.0);
      metric->update(_rrdb->get_tuples_cache(), ts + ii * (5 * INTERVAL_SEC) + 1, 1.0);
      TEST_CHECK_EQUAL(metric->is_dirty(), true);

      // save metric
      metric->save_dirty_blocks(_rrdb->get_files_cache(), _rrdb->get_journal_file());
      metric.reset();

      // write data to the journal file
      _rrdb->get_journal_file()->save_journal_file();

      // write data to the metric's file - we expect the metric file to already exist
      _rrdb->get_journal_file()->apply_journal(filename);

      // reset everything
      _rrdb->get_files_cache()->clear();
      _rrdb->get_tuples_cache()->clear();
      _rrdb->get_journal_file()->clear();
  }

  // done
  TEST_SUBTEST_END2("Done");
}

void update_tests::run(const std::string & path)
{
  // setup
  update_tests test;
  test.initialize(path);
  test.cleanup(10);

  // tests
  int n = 0;
  test.last_block_test(n++);

  test.load_test(n++,   1, 5, 50000);
  test.load_test(n++,   5, 5, 50000);
  test.load_test(n++, 10,  5, 50000);
  test.load_test(n++, 10, 50, 50000);

  // cleanup
  test.cleanup();
}
