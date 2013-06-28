/*
 * update_tests.h
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */

#ifndef UPDATE_TESTS_H_
#define UPDATE_TESTS_H_

#include <boost/shared_ptr.hpp>

#include "common/types.h"

class rrdb;

class update_tests
{
public:
  update_tests();
  virtual ~update_tests();

  static void run(const std::string & path);

private:
  void last_block_test(const int & n);

  void load_test(const int & n,
      const my::size_t & num_metrics,
      const my::size_t & num_threads,
      const my::size_t & num_tasks
  );

private:
  void initialize(const std::string & path);
  void cleanup(const my::size_t & num_metrics = 1);

private:
  boost::shared_ptr<rrdb> _rrdb;
};

#endif /* UPDATE_TESTS_H_ */
