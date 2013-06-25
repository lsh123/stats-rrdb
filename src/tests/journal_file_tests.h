/*
 * journal_file_tests.h
 *
 *  Created on: Jun 25, 2013
 *      Author: aleksey
 */

#ifndef JOURNAL_FILE_TESTS_H_
#define JOURNAL_FILE_TESTS_H_

#include <string>
#include <boost/shared_ptr.hpp>

#include "common/types.h"

class rrdb_files_cache;
class rrdb_journal_file;
class rrdb_metric_tuples_cache;

class journal_file_tests
{
public:
  journal_file_tests();
  virtual ~journal_file_tests();

  static void run(const std::string & path);

private:
  void initialize(const std::string & path);
  void cleanup();

  void test_apply_journal(const int & n);

private:
  boost::shared_ptr<rrdb_files_cache>  _files_cache;
  boost::shared_ptr<rrdb_journal_file> _journal_file;
  boost::shared_ptr<rrdb_metric_tuples_cache> _tuples_cache;
};

#endif /* JOURNAL_FILE_TESTS_H_ */
