/*
 * query_tests.h
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */

#ifndef RRDB_SELECT_TESTS_H_
#define RRDB_SELECT_TESTS_H_

#include <string>

#include <boost/shared_ptr.hpp>

class rrdb;

class query_tests
{
public:
  query_tests();
  virtual ~query_tests();

  static void run(const std::string & path);

private:
  void initialize(const std::string & path);
  void cleanup();

  void test_create(const int & n);
  void test_select_all(const int & n);
  void test_select_5_sec(const int & n);
  void test_select_all_group_by_30_sec(const int & n);
  void test_select_all_group_by_1_year(const int & n);

private:
  boost::shared_ptr<rrdb> _rrdb;
  std::string             _metric_name;
  my::time_t              _start_ts;
  my::time_t              _end_ts;
  my::time_t              _freq;
  my::size_t              _count;
}; // query_tests

#endif /* RRDB_SELECT_TESTS_H_ */
