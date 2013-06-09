/*
 * rrdb_metric.h
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_H_
#define RRDB_METRIC_H_

#include <string>
#include <fstream>

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

#include "spinlock.h"
#include "parser/retention_policy.h"

#define RRDB_METRIC_EXTENSION    ".rrdb"

// forward
class rrdb;

//
// RRDB Metric file header format:
//
// 0x99DB   - magic byte
// 0x0100   - version + status bytes
// 0xNNNN   - name length
// 0xNNNN   - policy size
// <name>   - aligned to 64 bytes
// (<64 bit freq> <64 bit count>)* - policy
//
class rrdb_metric
{
  enum status {
    Status_Deleted      = 0x01,
    Status_Dirty        = 0x02
  };

public:
  rrdb_metric();
  rrdb_metric(const std::string & name, const retention_policy & policy);
  virtual ~rrdb_metric();

  std::string get_name();
  retention_policy get_policy();

  bool is_dirty();
  void set_dirty();

  bool is_deleted();
  void set_deleted();

  void save_file(const std::string & folder);
  void delete_file(const std::string & folder);
  static boost::shared_ptr<rrdb_metric> load_file(const std::string & folder, const std::string & name);

private:
  static std::string get_full_path(const std::string & folder, const std::string & name);
  static std::size_t get_aligned_name_len(std::size_t name_len);

  void write_header(std::fstream & ofs);
  void read_header(std::fstream & ifs);

private:
  spinlock          _lock;
  std::string       _name;
  retention_policy  _policy;

  boost::uint8_t    _status;
}; // class rrdb_metric

#endif /* RRDB_METRIC_H_ */
