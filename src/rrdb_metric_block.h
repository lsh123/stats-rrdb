/*
 * rrdb_metric_block.h
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_BLOCK_H_
#define RRDB_METRIC_BLOCK_H_

#include <string>
#include <fstream>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/cstdint.hpp>

#include "spinlock.h"

//
// RRDB Metric Block Header format
//
typedef struct rrdb_metric_block_header_t_ {
  boost::uint16_t   _magic;             // magic bytes (0x99DB)
  boost::uint16_t   _status;            // block status flags
  boost::uint32_t   _unused1;

  boost::uint64_t   _offset;            // current offset in the file
  boost::uint64_t   _data_size;         // current size (bytes) in the file

  boost::uint32_t   _freq;              // frequency of collections in secs
  boost::uint32_t   _count;             // number of data tuples
  boost::uint32_t   _duration;          // for how long we store data (_freq * _count)
  boost::uint32_t   _pos;               // current position for circular buffer

  boost::int64_t    _pos_ts;            // current start time for this block
  boost::uint32_t   _unused2;
  boost::uint32_t   _unused3;
} rrdb_metric_block_header_t;

//
// Value
//
typedef struct rrdb_metric_tuple_t_ {
  boost::int64_t    _ts;                // block timestamp
  boost::int64_t    _count;             // number of data points aggregated
  double            _sum;               // sum(data point value)
  double            _sum_sqr;           // sum(sqr(data point value))
  double            _min;               // min(data point value)
  double            _max;               // max(data point value)
} rrdb_metric_tuple_t;


class rrdb_metric_block
{
  enum status {
    Status_Wrapped = 0x0001,
  };

public:
  rrdb_metric_block(boost::uint32_t freq = 0, boost::uint32_t count = 0, boost::uint64_t offset = 0);
  virtual ~rrdb_metric_block();

  inline boost::uint32_t get_freq() const {
    return _header._freq;
  }
  inline boost::uint32_t get_count() const {
    return _header._count;
  }
  inline boost::uint64_t get_offset() const {
    return _header._offset;
  }
  inline boost::uint64_t get_data_size() const {
    return _header._data_size;
  }
  inline boost::uint64_t get_size() const {
    return _header._data_size + sizeof(_header);
  }

  bool update(const boost::uint64_t & ts, const double & value);
  bool update(const rrdb_metric_tuple_t & values);

  void write_block(std::fstream & ofs);
  void read_block(std::fstream & ifs);

private:
  rrdb_metric_tuple_t * find_tuple(const boost::uint64_t & ts, bool & is_current_block);

  inline boost::uint64_t normalize_ts(const boost::uint64_t & ts) const {
    return ts - (ts % _header._freq);
  }

private:
  rrdb_metric_block_header_t               _header;
  boost::shared_array<rrdb_metric_tuple_t> _tuples;
}; // rrdb_metric_block

#endif /* RRDB_METRIC_BLOCK_H_ */
