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


class thread_pool_task
{
public:
  virtual void run() = 0;
}; // thread_pool_task

class thread_pool
{
public:
  thread_pool(std::size_t pool_size);
  virtual ~thread_pool();

  std::size_t run(boost::shared_ptr<thread_pool_task> task);

  inline double get_load_factor() const
  {
      return _used_threads.load(boost::memory_order_relaxed) / (double)_pool_size;
  }

private:
  void wrap_task_run(boost::shared_ptr<thread_pool_task> task);

private:
  std::size_t                    _pool_size;
  boost::asio::io_service        _io_service;
  boost::asio::io_service::work  _work;
  boost::thread_group            _threads;
  boost::atomic<std::size_t>     _used_threads;
};

#endif /* THREAD_POOL_H_ */
