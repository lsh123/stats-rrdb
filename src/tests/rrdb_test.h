/*
 * rrdb_test.h
 *
 *  Created on: Jun 17, 2013
 *      Author: aleksey
 */

#ifndef RRDB_TEST_H_
#define RRDB_TEST_H_

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "common/memory_buffer.h"
#include "common/types.h"

class rrdb;

class rrdb_test
{
public:
  typedef std::vector< std::string > params_t;
  typedef std::vector< std::vector<std::string> > csv_data_t;


public:
  rrdb_test(const boost::shared_ptr<rrdb> & rrdb);
  virtual ~rrdb_test();

  void run(const std::string & params_str);

  void run_lru_cache_test(const params_t & params);
  void run_select_test(const params_t & params);
  void run_perf_test(const params_t & params);

private:
  static params_t parse_params(const std::string & params_str);
  static std::string get_test_metric_name(const my::size_t & n);
  void cleanup(const my::size_t & metrics_num);

  static csv_data_t parse_csv_data(const t_memory_buffer_data & data);

private:
  boost::shared_ptr<rrdb> _rrdb;
}; // class rrdb_test

#endif /* RRDB_TEST_H_ */
