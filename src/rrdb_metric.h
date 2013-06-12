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
typedef struct rrdb_metric_header_t_ {
  boost::uint16_t   _magic;             // magic bytes (0x99DB)
  boost::uint16_t   _version;           // version (0x01)
  boost::uint16_t   _status;            // status flags
  boost::uint16_t   _name_len;          // length of the name (actual name is 64 bit padded with 0s)
  boost::uint16_t   _blocks_size;       // number of blocks in the metric
} rrdb_metric_header_t;

//
// RRDB Metric Block Info format
//
typedef struct rrdb_metric_block_info_t_ {
  boost::uint16_t   _magic;             // magic bytes (0x99DB)
  boost::uint16_t   _status;            // block status flags
  boost::uint32_t   _freq;              // frequency of collections in secs

  boost::uint32_t   _count;             // number of data tuples
  boost::uint32_t   _tuple_size;        // number of values in the tuple (const for now)

  boost::uint32_t   _start_pos;         // current start position
  boost::uint32_t   _end_pos;           // current end position

  boost::uint32_t   _start_ts;          // current start time for this block
  boost::uint32_t   _end_ts;            // current end time for this block

  boost::uint64_t   _offset;            // current offset in the file
} rrdb_metric_block_info;

//
// Value
//
typedef struct rrdb_metric_tuple_t_ {
  boost::uint64_t   _ts;                // block timestamp
  boost::uint64_t   _count;             // number of data points aggregated
  double            _min;               // min(data point value)
  double            _max;               // max(data point value)
  double            _sum;               // sum(data point value)
  double            _sum_sqr;           // sum(sqr(data point value))
} rrdb_metric_tuple_t;

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
  static boost::shared_ptr<rrdb_metric> load_file(const std::string & filename);

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
