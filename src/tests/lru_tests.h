/*
 * lru_tests.h
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */

#ifndef TESTS_LRU_TESTS_H_
#define TESTS_LRU_TESTS_H_

#include <string>

#include "common/lru_cache.h"
#include "common/types.h"

class lru_tests
{
  typedef lru_cache<std::string, std::string, my::time_t> t_test_lru_cache;

public:
  lru_tests();
  virtual ~lru_tests();

  // tests
  void test0_insert();
  void test1_find_and_use();
  void test2_erase();

public:
  static void run();


private:
  t_test_lru_cache _cache;
  my::time_t       _start_ts;
}; // lru_tests

#endif /* TESTS_LRU_TESTS_H_ */
