/*
 * rrdb_metric_block.cpp
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#include "rrdb/rrdb_metric_block.h"

#include "log.h"
#include "exception.h"

#define RRDB_METRIC_BLOCK_MAGIC    0xBB99

rrdb_metric_block::rrdb_metric_block(const rrdb_metric_block_pos_t & freq, const rrdb_metric_block_pos_t & count, const my::size_t & offset)
{
  memset(&_header, 0, sizeof(_header));
  _header._magic     = RRDB_METRIC_BLOCK_MAGIC;
  _header._freq      = freq;
  _header._count     = count;
  _header._duration  = _header._freq * _header._count;
  _header._offset    = offset;
  _header._data_size = _header._count * sizeof(rrdb_metric_tuple_t);

  if(_header._count > 0) {
      _tuples.reset(new rrdb_metric_tuple_t[_header._count]);
      memset(_tuples.get(), 0, _header._data_size);
  }
}

rrdb_metric_block::~rrdb_metric_block()
{
}

rrdb_metric_tuple_t * rrdb_metric_block::find_tuple(const update_ctx_t & in, update_ctx_t & out) {
  CHECK_AND_THROW(_tuples.get());
  CHECK_AND_THROW(_header._pos < _header._count);
  CHECK_AND_THROW(_header._freq > 0);

  const my::time_t & ts(in.get_ts());
  if(this->get_latest_possible_ts() <= ts) {
      // complete shift forward, notify about rollup
      out._state = UpdateState_Tuple;
      out._tuple = _tuples[_header._pos];

      // start from 0 and assume this ts is the latest
      memset(_tuples.get(), 0, _header._data_size);
      rrdb_metric_tuple_t & tuple = _tuples[0];
      _header._pos      = 0;
      _header._pos_ts   = tuple._ts = this->normalize_ts(ts);

      // done
      return &tuple;
  } else if(this->get_cur_ts() <= ts) {
      // we are somewhere ahead but not too much
      rrdb_metric_tuple_t * tuple = &_tuples[_header._pos];
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
          tuple = &_tuples[_header._pos];

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
          rrdb_metric_tuple_t * tuple = &_tuples[pos];
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

void rrdb_metric_block::update(const update_ctx_t & in, update_ctx_t & out)
{
  rrdb_metric_tuple_t * tuple = this->find_tuple(in, out);
  if(!tuple) {
      LOG(log::LEVEL_DEBUG, "Can not find tuple ts: %ld (current block time: %ld, duration: %ld)", in.get_ts(), this->get_cur_ts(), this->get_duration());
      return;
  }
  CHECK_AND_THROW(this->get_cur_ts() == this->get_cur_tuple()._ts);
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
}

bool rrdb_metric_block::select(const my::time_t & ts1, const my::time_t & ts2, rrdb::data_walker & walker) const
{
  CHECK_AND_THROW(_tuples.get());
  CHECK_AND_THROW(_header._pos < _header._count);

  // quick checks if this block overlaps with the [ts1, ts2] interval
  if(ts2 < this->get_earliest_ts()) {
      // select end time is earlier - try earlier blocks
      return true;
  }
  if(this->get_latest_ts() < ts1) {
      // select start time is greater than our end time - no point, stop
      return false;
  }

  // great this block overlaps with ts1/ts2 - walk through all
  // the tuples until we hit the end or
  rrdb_metric_block_pos_t pos(_header._pos);
  do {
      // check if walker is interested (i.e. if current tuple ts > ts1
      if(!walker.append(_tuples[pos], _header._freq)) {
          return false;
      }

      // move to prev one
      pos = this->get_prev_pos(pos);
  } while(pos != _header._pos);

  // try next block
  return true;
}

void rrdb_metric_block::write_block(std::fstream & ofs)
{
  CHECK_AND_THROW(_tuples.get());

  // write header
  ofs.write((const char*)&_header, sizeof(_header));

  // write data
  ofs.write((const char*)_tuples.get(), _header._data_size);
}

void rrdb_metric_block::read_block(std::fstream & ifs)
{
  CHECK_AND_THROW(!_tuples.get());

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
  _tuples.reset(new rrdb_metric_tuple_t[_header._count]);
  ifs.read((char*)_tuples.get(), _header._data_size);

  if(_tuples[_header._pos]._ts != _header._pos_ts) {
      throw exception("Unexpected rrdb metric block pos %u pos_ts: %ld (expected from tuple: %ld)",  _header._pos, _header._pos_ts, _tuples[_header._pos]._ts);
  }
}
