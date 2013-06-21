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
#include <boost/shared_array.hpp>
#include <boost/cstdint.hpp>

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric_tuple.h"

#include "types.h"
#include "spinlock.h"
#include "parser/retention_policy.h"


#define RRDB_METRIC_EXTENSION    ".rrdb"

// forward
class rrdb;
class rrdb_metric_block;
class statement_select;

//
// RRDB Metric file header format:
//
typedef struct rrdb_metric_header_t_ {
  boost::uint16_t   _magic;             // magic bytes (0x99DB)
  boost::uint16_t   _version;           // version (0x01)
  boost::uint16_t   _status;            // status flags
  boost::uint16_t   _blocks_size;       // number of blocks in the metric (size of _blocks array)

  my::value_t       _last_value;        // the latest value
  my::time_t        _last_value_ts;     // the ts of the latest value

  boost::uint16_t   _name_len;          // actual length of the name
  boost::uint16_t   _name_size;         // 64-bit padded size of the name array
  boost::uint16_t   _unused1;
  boost::uint16_t   _unused2;
} rrdb_metric_header_t;

//
//
//
class rrdb_metric
{
  enum status {
    Status_Deleted      = 0x01,
    Status_Dirty        = 0x02
  };


public:
  rrdb_metric(const std::string & filename = std::string());
  virtual ~rrdb_metric();

  std::string get_name();
  retention_policy get_policy();
  void set_name_and_policy(const std::string & name, const retention_policy & policy);

  bool is_dirty();
  void set_dirty();

  bool is_deleted();
  void set_deleted();

  void load_file(const rrdb * const rrdb);
  void save_file(const rrdb * const rrdb, const std::string & folder);
  void delete_file(const rrdb * const rrdb, const std::string & folder);

  void update(const rrdb * const rrdb, const my::time_t & ts, const my::value_t & value);
  void select(const rrdb * const rrdb, const my::time_t & ts1, const my::time_t & ts2, rrdb::data_walker & walker);

  void get_last_value(my::value_t & value, my::time_t & value_ts);

  static std::string get_filename(const std::string & name);
  static void initialize_subfolders(const std::string & path);

private:
  std::string get_full_path(const rrdb * const rrdb);
  static my::size_t get_padded_name_len(const my::size_t & name_len);

  void write_header(std::fstream & ofs);
  void read_header(std::fstream & ifs);

private:
  spinlock                       _lock;
  rrdb_metric_header_t           _header;
  boost::shared_array<char>      _name;
  std::vector<rrdb_metric_block> _blocks;

  std::string                    _filename;
}; // class rrdb_metric

#endif /* RRDB_METRIC_H_ */
