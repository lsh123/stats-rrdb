/*
 * thread_pool.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <string>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/atomic.hpp>

#include "common/types.h"

class thread_pool_task
{
public:
  virtual void run() = 0;
}; // thread_pool_task

class thread_pool
{
public:
  thread_pool(my::size_t pool_size);
  virtual ~thread_pool();

  my::size_t run(boost::shared_ptr<thread_pool_task> task);

  inline double get_load_factor() const
  {
      return _used_threads.load(boost::memory_order_relaxed) / (double)_pool_size;
  }

  inline my::size_t get_started_jobs() const
  {
      return _started_jobs.load(boost::memory_order_relaxed);
  }

  inline my::size_t get_finished_jobs() const
  {
      return _finished_jobs.load(boost::memory_order_relaxed);
  }

private:
  void wrap_task_run(boost::shared_ptr<thread_pool_task> task);

private:
  my::size_t                    _pool_size;
  boost::asio::io_service        _io_service;
  boost::asio::io_service::work  _work;
  boost::thread_group            _threads;
  boost::atomic<my::size_t>     _used_threads;
  boost::atomic<my::size_t>     _started_jobs;
  boost::atomic<my::size_t>     _finished_jobs;
};

#endif /* THREAD_POOL_H_ */
