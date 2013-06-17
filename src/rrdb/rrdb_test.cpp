/*
 * rrdb_test.cpp
 *
 *  Created on: Jun 17, 2013
 *      Author: aleksey
 */
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "rrdb/rrdb_test.h"
#include "rrdb/rrdb.h"

#include "server/thread_pool.h"

#include "exception.h"


rrdb_test::rrdb_test(boost::shared_ptr<rrdb> rrdb, const std::string & cmd) :
  _rrdb(rrdb),
  _cmd(cmd)
{
  // TODO Auto-generated constructor stub

}

rrdb_test::~rrdb_test()
{
  // TODO Auto-generated destructor stub
}

void rrdb_test::run()
{
  memory_buffer_data_t output_buffer;
  memory_buffer_t res(output_buffer);

  _rrdb->execute_udp_command(_cmd, res);
  res.flush();
}

// params string format: <threads>|<tasks>|<metrics>
void rrdb_test::run_test(boost::shared_ptr<rrdb> rrdb, const std::string & params) {
  // parse params string
  std::vector< std::string > data;
  boost::algorithm::split(data, params, boost::algorithm::is_any_of("|"), boost::algorithm::token_compress_off);
  if(data.size() != 3) {
      throw exception("Invalid test params: expected <threads>|<tasks>|<metrics>");
  }
  std::size_t threads_num = boost::lexical_cast<std::size_t>(data[0]);
  std::size_t tasks_num   = boost::lexical_cast<std::size_t>(data[1]);
  std::size_t metrics_num = boost::lexical_cast<std::size_t>(data[2]);

  // run test
  std::cout << "Starting performance test with "
    << threads_num << " threads, "
    << tasks_num << " tasks, "
    << metrics_num << " metrics"
    << std::endl;
  boost::shared_ptr<thread_pool> threads(new thread_pool(threads_num));
  char buf[1024];
  time_t ts1(time(NULL));
  for(std::size_t ii = 0; ii < tasks_num; ii++) {
      snprintf(buf, sizeof(buf), "u|test.metric.%lu|1|%lu", rand() % metrics_num, ts1 + 1000*ii);
      boost::shared_ptr<rrdb_test> t(new rrdb_test(rrdb, buf));

      threads->run(t);

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

  // done
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

  // cleanup
  for(std::size_t ii = 0; ii < metrics_num; ii++) {
      snprintf(buf, sizeof(buf), "d|test.metric.%lu", ii);
      boost::shared_ptr<rrdb_test> t(new rrdb_test(rrdb, buf));

      threads->run(t);
  }
  std::cout << "Cleaned up, done!" << std::endl;
}
