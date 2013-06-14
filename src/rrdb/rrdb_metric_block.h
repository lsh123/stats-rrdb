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
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/cstdint.hpp>

#include "spinlock.h"
#include "exception.h"
#include "parser/interval.h"
#include "rrdb/rrdb_metric_tuple.h"

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
// Metrics block of data for a single policy
//
class rrdb_metric_block
{
  enum status {
    Status_Wrapped = 0x0001
  };

public:
  enum update_state {
    UpdateState_Stop  = 0,
    UpdateState_Value = 1,
    UpdateState_Tuple = 2
  };

  typedef struct update_ctx_t_ {
    enum update_state   _state;

    boost::uint64_t     _ts;
    double              _value;
    rrdb_metric_tuple_t _tuple;

    inline boost::int64_t get_ts() const {
      switch(_state) {
      case UpdateState_Value:
        return _ts;
      case UpdateState_Tuple:
        return _tuple._ts;
      default:
        throw new exception("Unexpected update ctx state %d", _state);
      }
    }
  } update_ctx_t;

  class select_ctx {
  public:
    virtual void append(const rrdb_metric_tuple_t & tuple, const interval_t & interval) = 0;

  public:
    boost::int64_t _ts_begin;
    boost::int64_t _ts_end;
  }; // select_ctx

public:
  rrdb_metric_block(boost::uint32_t freq = 0, boost::uint32_t count = 0, boost::uint64_t offset = 0);
  virtual ~rrdb_metric_block();

  // DATA STUFF
  inline boost::uint64_t get_offset() const {
    return _header._offset;
  }
  inline boost::uint64_t get_data_size() const {
    return _header._data_size;
  }
  inline boost::uint64_t get_size() const {
    return _header._data_size + sizeof(_header);
  }

  // BLOCK POLICY STUFF
  inline boost::uint32_t get_freq() const {
    return _header._freq;
  }
  inline boost::uint32_t get_count() const {
    return _header._count;
  }
  inline boost::uint32_t get_duration() const {
    return _header._duration;
  }

  // TIMESTAMPS STUFF
  inline boost::int64_t get_cur_ts() const {
    return _header._pos_ts;
  }
  inline boost::int64_t get_earliest_ts() const {
    return _header._pos_ts - _header._duration - _header._freq;
  }
  inline boost::int64_t get_latest_ts() const {
    return _header._pos_ts + _header._freq;
  }
  inline boost::int64_t get_latest_possible_ts() const {
    return _header._pos_ts + _header._duration;
  }

  // SELECT or UPDATE - main operations
  bool select(select_ctx & ctx) const;
  void update(const update_ctx_t & in, update_ctx_t & out);

  // READ/WRITE FILES
  void write_block(std::fstream & ofs);
  void read_block(std::fstream & ifs);

private:
  rrdb_metric_tuple_t * find_tuple(const update_ctx_t & in, update_ctx_t & out);

  inline boost::uint64_t normalize_ts(const boost::uint64_t & ts) const {
    return ts - (ts % _header._freq);
  }

  inline boost::uint32_t get_next_pos(const boost::uint32_t & pos) const {
    boost::uint32_t new_pos = pos + 1;
    return (new_pos < _header._count) ? new_pos : 0;
  }
  inline boost::uint32_t get_prev_pos(const boost::uint32_t & pos) const {
    return (pos > 0) ? (pos - 1) : (_header._count - 1);
  }

private:
  rrdb_metric_block_header_t               _header;
  boost::shared_array<rrdb_metric_tuple_t> _tuples;
}; // rrdb_metric_block

#endif /* RRDB_METRIC_BLOCK_H_ */
