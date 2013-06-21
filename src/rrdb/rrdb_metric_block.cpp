/*
 * rrdb_metric_block.cpp
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#include "rrdb/rrdb_metric_block.h"

#include "types.h"
#include "log.h"
#include "exception.h"

#define RRDB_METRIC_BLOCK_MAGIC    0xBB99

rrdb_metric_block::rrdb_metric_block(
    const rrdb_metric_block_pos_t & freq,
    const rrdb_metric_block_pos_t & count,
    const my::size_t & offset
)
{
  memset(&_header, 0, sizeof(_header));
  _header._magic     = RRDB_METRIC_BLOCK_MAGIC;
  _header._freq      = freq;
  _header._count     = count;
  _header._duration  = _header._freq * _header._count;
  _header._offset    = offset;
  _header._data_size = _header._count * sizeof(rrdb_metric_tuple_t);

  if(_header._count > 0) {
      _tuples_data.reset(new rrdb_metric_tuple_t[_header._count]);
      memset(_tuples_data.get(), 0, _header._data_size);
  }
}

rrdb_metric_block::~rrdb_metric_block()
{
}


boost::shared_array<rrdb_metric_tuple_t> rrdb_metric_block::get_tuples(
    const rrdb * const rrdb,
    const rrdb_metric * const rrdb_metric
) const
{
  CHECK_AND_THROW(rrdb);
  CHECK_AND_THROW(rrdb_metric);

  return _tuples_data;
}

rrdb_metric_tuple_t * rrdb_metric_block::find_tuple(
    rrdb_metric_tuple_t * the_tuples,
    const update_ctx_t & in,
    update_ctx_t & out
) {
  CHECK_AND_THROW(the_tuples);
  CHECK_AND_THROW(_header._pos < _header._count);
  CHECK_AND_THROW(_header._freq > 0);

  const my::time_t & ts(in.get_ts());
  if(this->get_latest_possible_ts() <= ts) {
      // complete shift forward, notify about rollup
      out._state = UpdateState_Tuple;
      out._tuple = the_tuples[_header._pos];

      // start from 0 and assume this ts is the latest
      memset(the_tuples, 0, _header._data_size);
      rrdb_metric_tuple_t & tuple = the_tuples[0];
      _header._pos      = 0;
      _header._pos_ts   = tuple._ts = this->normalize_ts(ts);

      // done
      return &tuple;
  } else if(this->get_cur_ts() <= ts) {
      // we are somewhere ahead but not too much
      rrdb_metric_tuple_t * tuple = &the_tuples[_header._pos];
      my::time_t next_tuple_ts = tuple->_ts + _header._freq;
      if(ts < next_tuple_ts) {
          // our current tuple will do
          out._state = UpdateState_Stop;
          return tuple;
      }

      // we are shifting forward, notify about rollup
      out._state = UpdateState_Tuple;
      out._tuple = (*tuple);

      // find the tuple
      do {
          // move the pointer
          _header._pos = this->get_next_pos(_header._pos);
          tuple = &the_tuples[_header._pos];

          // reset values
          memset(tuple, 0, sizeof(*tuple));
          tuple->_ts = _header._pos_ts = next_tuple_ts;

          // LOG(log::LEVEL_DEBUG, "Find tuple %p: new pos: %lu, new tuple ts: %lld", this, _header._pos, next_tuple_ts);

          // check next
          next_tuple_ts += _header._freq;
      } while(next_tuple_ts <= ts);

      // done
      return tuple;
  } else if(this->get_earliest_ts() <= ts) {
      // we are somewhere behind but not too much, just update the same way
      out = in;

      // find the spot
      my::time_t tuple_ts = this->get_cur_ts();
      for(rrdb_metric_block_pos_t pos = this->get_prev_pos(_header._pos); pos != _header._pos; pos = this->get_prev_pos(pos)) {
          tuple_ts -= _header._freq;

          // overwrite tuple ts just in case (it might not be initialized!)
          rrdb_metric_tuple_t * tuple = &the_tuples[pos];
          tuple->_ts = tuple_ts;
          if(ts >= tuple->_ts) {
              CHECK_AND_THROW(ts < tuple->_ts + _header._freq);
              return tuple;
          }
      }

      // shouldn't happen really but just in case
      return NULL;
  } else {
      // we are WAY behind, ignore but let the next block try
      out = in;

      // done
      CHECK_AND_THROW(ts < this->get_earliest_ts());
      return NULL;
  }
}

void rrdb_metric_block::update(
    const rrdb * const rrdb,
    const rrdb_metric * const rrdb_metric,
    const update_ctx_t & in,
    update_ctx_t & out
)
{
  boost::shared_array<rrdb_metric_tuple_t> the_tuples(this->get_tuples(rrdb, rrdb_metric));
  CHECK_AND_THROW(the_tuples.get());
  CHECK_AND_THROW(this->get_cur_ts() == the_tuples[_header._pos]._ts);

  rrdb_metric_tuple_t * tuple = this->find_tuple(the_tuples.get(), in, out);
  if(!tuple) {
      LOG(log::LEVEL_DEBUG, "Can not find tuple ts: %ld (current block time: %ld, duration: %ld)", in.get_ts(), this->get_cur_ts(), this->get_duration());
      return;
  }
  CHECK_AND_THROW(tuple->_ts <= in.get_ts());
  CHECK_AND_THROW(in.get_ts() < tuple->_ts + _header._freq);

  // update our tuple
  switch(in._state) {
  case UpdateState_Value:
    // we have single value
    rrdb_metric_tuple_update(*tuple, in._value);
    break;
  case UpdateState_Tuple:
    // we have another tuple
    rrdb_metric_tuple_update(*tuple, in._tuple);
    break;
  default:
    throw exception("Unexpected update ctx state %d", in._state);
  }

  // mark dirty
  my::bitmask_set<boost::uint16_t>(_header._status, Status_Dirty);
}

void rrdb_metric_block::select(
    const rrdb * const rrdb,
    const rrdb_metric * const rrdb_metric,
    const my::time_t & ts1,
    const my::time_t & ts2,
    rrdb::data_walker & walker
) const
{
  CHECK_AND_THROW(_header._pos < _header._count);

  boost::shared_array<rrdb_metric_tuple_t> the_tuples(this->get_tuples(rrdb, rrdb_metric));
  CHECK_AND_THROW(the_tuples.get());

  // walk through all the tuples until we hit the end or the time stops
  // note that logic for checking timestamps in rrdb_metric::select()
  // is very similar
  rrdb_metric_block_pos_t pos(_header._pos);
  do {
      const rrdb_metric_tuple_t & tuple(the_tuples[pos]);
      int res = my::interval_overlap(tuple._ts, tuple._ts + _header._freq, ts1, ts2);
      if(res < 0) {
          // [tuple) < [ts1, ts2): tuples are ordered from newest to oldest, so we
          // are done - all the next tuples will be earlier than this one
          break;
      }
      if(res ==  0) {
          // res == 0 => tuple and interval intersect!
          walker.append(tuple, _header._freq);
      }

      // move to prev one
      pos = this->get_prev_pos(pos);
  } while(pos != _header._pos);
}

void rrdb_metric_block::write_block(
    const rrdb * const rrdb,
    const rrdb_metric * const rrdb_metric,
    std::fstream & ofs
)
{
  boost::shared_array<rrdb_metric_tuple_t> the_tuples(this->get_tuples(rrdb, rrdb_metric));
  CHECK_AND_THROW(the_tuples.get());

  try {
    // not dirty (clear before writing header)
    my::bitmask_clear<boost::uint16_t>(_header._status, Status_Dirty);

    // write header
    ofs.write((const char*)&_header, sizeof(_header));

    // write data
    ofs.write((const char*)the_tuples.get(), _header._data_size);
  } catch(...) {
      // well, still dirty
      my::bitmask_set<boost::uint16_t>(_header._status, Status_Dirty);
      throw;
  }
}

void rrdb_metric_block::read_block(
    const rrdb * const rrdb,
    const rrdb_metric * const rrdb_metric,
    std::fstream & ifs
)
{
  // rememeber where are we
  uint64_t offset = ifs.tellg();

  // read header
  ifs.read((char*)&_header, sizeof(_header));
  if(_header._magic != RRDB_METRIC_BLOCK_MAGIC) {
      throw exception("Unexpected rrdb metric block magic: %04x", _header._magic);
  }
  if(_header._freq <= 0) {
      throw exception("Unexpected rrdb metric block frequency: %u", _header._freq);
  }
  if(_header._count <= 0) {
      throw exception("Unexpected rrdb metric block count: %u", _header._count);
  }
  if(_header._duration != _header._freq * _header._count) {
      throw exception("Unexpected rrdb metric block duration: %u (expected %u)", _header._duration, _header._freq * _header._count);
  }
  if(_header._pos >= _header._count) {
      throw exception("Unexpected rrdb metric block pos: %u", _header._pos);
  }
  if(_header._offset != offset) {
      throw exception("Unexpected rrdb metric block offset: %lu (expected %lu)", _header._offset, offset);
  }
  if(_header._data_size != _header._count * sizeof(rrdb_metric_tuple_t)) {
      throw exception("Unexpected rrdb metric block data size: %lu (expected %su)", _header._data_size, _header._count * sizeof(rrdb_metric_tuple_t));
  }

  // read data
  _tuples_data = this->read_block_data(ifs);
}

boost::shared_array<rrdb_metric_tuple_t> rrdb_metric_block::read_block_data(std::fstream & ifs)
{
  // read data
  boost::shared_array<rrdb_metric_tuple_t> the_tuples(new rrdb_metric_tuple_t[_header._count]);
  ifs.read((char*)the_tuples.get(), _header._data_size);

  // check data
  if(the_tuples[_header._pos]._ts != _header._pos_ts) {
      throw exception("Unexpected rrdb metric block pos %u pos_ts: %ld (expected from tuple: %ld)",  _header._pos, _header._pos_ts, the_tuples[_header._pos]._ts);
  }

  // done
  return the_tuples;
}
