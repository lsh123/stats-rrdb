/*
 * rrdb_test.h
 *
 *  Created on: Jun 17, 2013
 *      Author: aleksey
 */

#ifndef RRDB_TEST_H_
#define RRDB_TEST_H_


#include <boost/shared_ptr.hpp>

#include "server/thread_pool.h"

class rrdb;

class rrdb_test:
    public thread_pool_task
{
public:
  rrdb_test(boost::shared_ptr<rrdb> rrdb, const std::string & cmd);
  virtual ~rrdb_test();

  static void run_test(boost::shared_ptr<rrdb> rrdb, const std::string & params);

public:
  // thread_pool_task
  virtual void run();

private:
  boost::shared_ptr<rrdb> _rrdb;
  std::string             _cmd;
}; // class rrdb_test

#endif /* RRDB_TEST_H_ */
