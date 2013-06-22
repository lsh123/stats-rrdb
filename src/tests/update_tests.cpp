/*
 * update_tests.cpp
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */
#include "tests/update_tests.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric.h"

#include "server/thread_pool.h"

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

      _rrdb->execute_udp_command(_cmd, res);
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
  config_data["log.level"] = "info";
  config_data["log.destination"] = path + "./query_tests.log";

  //
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
  TEST_SUBTEST_START(n, buf);

  // cleanup locally
  this->cleanup(num_metrics);

  my::time_t ts(time(NULL));
  boost::shared_ptr<thread_pool> threads(new thread_pool(num_threads));
  boost::posix_time::ptime ts1 = boost::posix_time::microsec_clock::local_time();
  for(my::size_t ii = 0; ii < num_tasks; ii++) {
      snprintf(buf, sizeof(buf),  "u|" METRIC_NAME_TEMPLATE "|1|%lu",
          rand() % num_metrics,
          ts + ii
      );

      boost::shared_ptr<load_test_task> task(new load_test_task(_rrdb, buf));
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

void update_tests::run(const std::string & path)
{
  // setup
  update_tests test;
  test.initialize(path);
  test.cleanup(10);

  // tests
  test.load_test(0,   1, 5, 20000);
  test.load_test(1,   5, 5, 20000);
  test.load_test(2, 10,  5, 20000);
  test.load_test(3, 10, 50, 20000);

  // cleanup
  test.cleanup();
}
