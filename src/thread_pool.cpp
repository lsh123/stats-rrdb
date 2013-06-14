/*
 * thread_pool.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#include "exception.h"
#include "log.h"
#include "thread_pool.h"

thread_pool::thread_pool(std::size_t pool_size) :
  _pool_size(pool_size),
  _work(_io_service),
  _used_threads(0)
{
  LOG(log::LEVEL_DEBUG, "Creating thread pool");

  // init
  for(std::size_t ii = 0; ii < _pool_size; ++ii) {
      _threads.create_thread(boost::bind(&boost::asio::io_service::run, &_io_service));
  }

  // done
  LOG(log::LEVEL_INFO, "Created thread pool with %lu threads", SIZE_T_CAST _pool_size);
}

thread_pool::~thread_pool()
{
  LOG(log::LEVEL_DEBUG, "Stopping thread pool");

  // Force all threads to return from io_service::run().
  _io_service.stop();

  // Suppress all exceptions.
  try {
      _threads.join_all();
  } catch (std::exception& e) {
    LOG(log::LEVEL_ERROR, "%s", e.what());
  }

  LOG(log::LEVEL_INFO, "Stopped thread pool");
}

void thread_pool::run(boost::shared_ptr<thread_pool_task> task)
{
  // ready to execute?
  if((++_used_threads) > _pool_size) {
    --_used_threads;
    throw new exception("No available threads to execute the task");
  }

  //
  LOG(log::LEVEL_DEBUG3, "Starting task, %lu out of %lu thread(s) used", SIZE_T_CAST _used_threads, SIZE_T_CAST _pool_size);

  // go!
  _io_service.post(boost::bind( &thread_pool::wrap_task_run, this, task)) ;
}

void thread_pool::wrap_task_run(boost::shared_ptr<thread_pool_task> task)
{
  // run the task, ignore all exception
  try {
      task->run();
  } catch(std::exception & e) {
      LOG(log::LEVEL_ERROR, "%s", e.what());
  } catch(...) {
      LOG(log::LEVEL_ERROR, "unknown exception");
  }

  // done!
  --_used_threads;

  //
  LOG(log::LEVEL_DEBUG3, "Finished task, %lu out of %lu thread(s) used", SIZE_T_CAST _used_threads, SIZE_T_CAST _pool_size);
}


