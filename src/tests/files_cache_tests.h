/*
 * files_cache_tests.h
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */

#ifndef FILES_CACHE_TESTS_H_
#define FILES_CACHE_TESTS_H_

#include <string>

#include <boost/shared_ptr.hpp>

#include "common/types.h"

class rrdb_files_cache;

class files_cache_tests
{
public:
  files_cache_tests();
  virtual ~files_cache_tests();

  static void run(const std::string & path);

private:
  void initialize(const std::string & path);
  void cleanup();

  void test_open_file(const int & n);
  void test_delete_file(const int & n);

  static my::filename_t  get_filename(int n);

private:
  boost::shared_ptr<rrdb_files_cache> _files_cache;
}; // files_cache_tests

#endif /* FILES_CACHE_TESTS_H_ */
