/*
 * thread_pool.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/detail/atomic_count.hpp>


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

  void run(boost::shared_ptr<thread_pool_task> task);

private:
  void wrap_task_run(boost::shared_ptr<thread_pool_task> task);

private:
  boost::asio::io_service       _io_service;
  boost::asio::io_service::work _work;
  boost::thread_group           _threads;
  boost::detail::atomic_count   _used_threads;
  std::size_t                   _pool_size;
};

#endif /* THREAD_POOL_H_ */
