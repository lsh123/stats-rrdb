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

#include "common/types.h"
#include "common/spinlock.h"
#include "common/enable_intrusive_ptr.h"
#include "common/exception.h"


class rrdb;
class rrdb_metric;
class rrdb_metric_tuples_cache;
class rrdb_files_cache;

typedef boost::uint32_t rrdb_metric_block_pos_t;

//
// RRDB Metric Block Header format
//
typedef struct t_rrdb_metric_block_header_ {
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
} t_rrdb_metric_block_header;


//
// Metrics block of data for a single policy
//
class rrdb_metric_block:
    public enable_intrusive_ptr<rrdb_metric_block>
{
public:
  enum update_state {
    UpdateState_Stop  = 0,
    UpdateState_Value = 1,
    UpdateState_Tuple = 2
  };

  typedef struct t_update_ctx_ {
    enum update_state   _state;

    my::time_t          _ts;
    my::value_t         _value;
    t_rrdb_metric_tuple _tuple;

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
  } t_update_ctx;


public:
  rrdb_metric_block(
      const rrdb_metric_block_pos_t & freq = 0,
      const rrdb_metric_block_pos_t & count = 0,
      const my::size_t & offset = 0
  );
  virtual ~rrdb_metric_block();

  // STATUS STUFF
  inline bool is_dirty() const
  {
    return _modified_tuples;
  }

  // DATA STUFF
  inline my::size_t get_offset() const {
    return _header._offset;
  }
  inline my::size_t get_offset_to_data() const {
    return _header._offset + sizeof(_header);
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

  //
  // SELECT or UPDATE - main operations: should be called by the rrdb_metric only
  //
  void select(
      const my::filename_t & filename,
      const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache,
      const my::time_t & ts1,
      const my::time_t & ts2,
      rrdb::data_walker & walker
  );
  void update(
      const my::filename_t & filename,
      const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache,
      const t_update_ctx & in,
      t_update_ctx & out
  );

  // READ/WRITE FILES
  void write_block(std::fstream & ofs);
  void read_block(std::fstream & ifs, bool skip_data = true);

private:
  t_rrdb_metric_tuples_ptr get_tuples(
      const my::filename_t & filename,
      const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache
  );

  t_rrdb_metric_tuple * find_tuple(
      t_rrdb_metric_tuples_ptr & the_tuples,
      const t_update_ctx & in,
      t_update_ctx & out
  );

  t_rrdb_metric_tuples_ptr read_block_data(
      std::fstream & ifs
  ) const;

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
  t_rrdb_metric_block_header  _header;
  t_rrdb_metric_tuples_ptr    _modified_tuples;
}; // rrdb_metric_block

#endif /* RRDB_METRIC_BLOCK_H_ */
