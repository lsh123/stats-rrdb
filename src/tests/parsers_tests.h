/*
 * parsers_tests.h
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */

#ifndef PARSERS_TESTS_H_
#define PARSERS_TESTS_H_

#include <string>
#include "common/types.h"

class parsers_tests
{
public:
  parsers_tests();
  virtual ~parsers_tests();

  // tests
  void test_interval(const int & n);
  void test_retention_policy(const int & n);
  void test_memory_size(const int & n);

public:
  static void run();
};

#endif /* PARSERS_TESTS_H_ */
