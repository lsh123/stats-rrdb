/*
 * thread_pool.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#include "exception.h"
#include "log.h"
#include "config.h"
#include "thread_pool.h"

thread_pool::thread_pool(boost::shared_ptr<config> config) :
   _work(_io_service),
   _used_threads(0)
{
  // load configs
  _pool_size = config->get<std::size_t>("server.thread_pool_size", 10);

  // init
  for(std::size_t ii = 0; ii < _pool_size; ++ii) {
      _threads.create_thread(boost::bind(&boost::asio::io_service::run, &_io_service));
  }

  // done
  log::write(log::LEVEL_INFO, "Created thread pool with %zu threads", _pool_size);
}

thread_pool::~thread_pool()
{
  // Force all threads to return from io_service::run().
  _io_service.stop();

  // Suppress all exceptions.
  try {
      _threads.join_all();
  } catch (std::exception& e) {
    log::write(log::LEVEL_ERROR, "%s", e.what());
  }
}

void thread_pool::run(boost::shared_ptr<thread_pool_task> task)
{
  // ready to execute?
  if((++_used_threads) > _pool_size) {
    --_used_threads;
    throw new exception("No available threads to execute the task");
  }

  //
  log::write(log::LEVEL_DEBUG, "Starting task, %lli out of %zu thread(s) used", (long int)_used_threads, _pool_size);

  // go!
  _io_service.post(boost::bind( &thread_pool::wrap_task_run, this, task)) ;
}

void thread_pool::wrap_task_run(boost::shared_ptr<thread_pool_task> task)
{
  // run the task, ignore all exception
  try {
      task->run();
  } catch(std::exception & e) {
      log::write(log::LEVEL_ERROR, "%s", e.what());
  } catch(...) {
      log::write(log::LEVEL_ERROR, "unknown exception");
  }

  // done!
  --_used_threads;

  //
  log::write(log::LEVEL_DEBUG, "Finished task, %lli out of %zu thread(s) used",(long int) _used_threads, _pool_size);
}


