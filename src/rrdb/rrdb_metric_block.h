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

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric_tuple.h"

#include "types.h"
#include "spinlock.h"
#include "exception.h"


typedef boost::uint32_t rrdb_metric_block_pos_t;

//
// RRDB Metric Block Header format
//
typedef struct rrdb_metric_block_header_t_ {
  boost::uint16_t           _magic;             // magic bytes (0x99DB)
  boost::uint16_t           _status;            // block status flags
  boost::uint32_t           _unused1;

  my::size_t                _offset;            // current offset in the file
  my::size_t                _data_size;         // current size (bytes) in the file

  my::interval_t            _freq;             // frequency of collections in secs
  my::interval_t            _duration;          // for how long we store data (_freq * _count)
  rrdb_metric_block_pos_t   _count;             // number of data tuples
  rrdb_metric_block_pos_t   _pos;               // current position for circular buffer

  my::time_t                _pos_ts;            // current start time for this block
  boost::uint32_t           _unused2;
  boost::uint32_t           _unused3;
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

    my::time_t          _ts;
    my::value_t         _value;
    rrdb_metric_tuple_t _tuple;

    inline my::time_t get_ts() const {
      switch(_state) {
      case UpdateState_Value:
        return _ts;
      case UpdateState_Tuple:
        return _tuple._ts;
      default:
        throw exception("Unexpected update ctx state %d", _state);
      }
    }
  } update_ctx_t;


public:
  rrdb_metric_block(const rrdb_metric_block_pos_t & freq = 0, const rrdb_metric_block_pos_t & count = 0, const my::size_t & offset = 0);
  virtual ~rrdb_metric_block();

  // DATA STUFF
  inline my::size_t get_offset() const {
    return _header._offset;
  }
  inline my::size_t get_data_size() const {
    return _header._data_size;
  }
  inline my::size_t get_size() const {
    return _header._data_size + sizeof(_header);
  }

  // BLOCK POLICY STUFF
  inline my::interval_t get_freq() const {
    return _header._freq;
  }
  inline my::interval_t get_duration() const {
    return _header._duration;
  }
  inline rrdb_metric_block_pos_t get_count() const {
    return _header._count;
  }

  // tuple
  rrdb_metric_tuple_t & get_cur_tuple() {
    return _tuples[_header._pos];
  }

  // TIMESTAMPS STUFF
  inline my::time_t get_cur_ts() const {
    return _header._pos_ts;
  }
  inline my::time_t get_earliest_ts() const {
    return _header._pos_ts - _header._duration - _header._freq;
  }
  inline my::time_t get_latest_ts() const {
    return _header._pos_ts + _header._freq;
  }
  inline my::time_t get_latest_possible_ts() const {
    return _header._pos_ts + _header._duration;
  }

  // SELECT or UPDATE - main operations
  bool select(const my::time_t & ts1, const my::time_t & ts2, rrdb::data_walker & walker) const;
  void update(const update_ctx_t & in, update_ctx_t & out);

  // READ/WRITE FILES
  void write_block(std::fstream & ofs);
  void read_block(std::fstream & ifs);

private:
  rrdb_metric_tuple_t * find_tuple(const update_ctx_t & in, update_ctx_t & out);

  inline my::time_t normalize_ts(const my::time_t & ts) const {
    return ts - (ts % _header._freq);
  }

  inline rrdb_metric_block_pos_t get_next_pos(const rrdb_metric_block_pos_t & pos) const {
    rrdb_metric_block_pos_t new_pos = pos + 1;
    return (new_pos < _header._count) ? new_pos : 0;
  }
  inline rrdb_metric_block_pos_t get_prev_pos(const rrdb_metric_block_pos_t & pos) const {
    return (pos > 0) ? (pos - 1) : (_header._count - 1);
  }

private:
  rrdb_metric_block_header_t               _header;
  boost::shared_array<rrdb_metric_tuple_t> _tuples;
}; // rrdb_metric_block

#endif /* RRDB_METRIC_BLOCK_H_ */
