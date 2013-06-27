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
  void test_memory_size(const int & n);
  void test_interval(const int & n);
  void test_retention_policy(const int & n);
  void test_statement_create(const int & n);
  void test_statement_drop(const int & n);
  void test_statement_update(const int & n);
  void test_statement_select(const int & n);
  void test_statement_show_policy(const int & n);
  void test_statement_show_metrics(const int & n);
  void test_statement_show_status(const int & n);


public:
  static void run();
};

#endif /* PARSERS_TESTS_H_ */
