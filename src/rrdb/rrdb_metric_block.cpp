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

rrdb_metric_block::rrdb_metric_block(boost::uint32_t freq, boost::uint32_t count, boost::uint64_t offset)
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
  CHECK_AND_LOG2(_tuples.get(), NULL);
  CHECK_AND_LOG2(_header._pos < _header._count, NULL);
  CHECK_AND_LOG2(_header._freq > 0, NULL);

  const boost::uint64_t & ts(in.get_ts());
  if(_header._pos_ts + _header._duration <= ts) {
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
  } else if(_header._pos_ts <= ts) {
      // we are somewhere ahead but not too much
      rrdb_metric_tuple_t * tuple = &_tuples[_header._pos];
      boost::uint64_t next_tuple_ts = tuple->_ts + _header._freq;
      if(ts < next_tuple_ts) {
          // our current tuple will do
          out._state = UpdateState_Stop;
          return tuple;
      }

      // we are shifting forward, notify about rollup
      out._state = UpdateState_Tuple;
      out._tuple = _tuples[_header._pos];

      // find the tuple
      do {
          // move the pointer
          _header._pos = this->get_next_pos(_header._pos);
          tuple = &_tuples[_header._pos];

          // reset values
          memset(tuple, 0, sizeof(*tuple));
          tuple->_ts = _header._pos_ts = next_tuple_ts;

          // log::write(log::LEVEL_DEBUG, "Find tuple %p: new pos: %lu, new tuple ts: %lld", this, _header._pos, next_tuple_ts);

          // check next
          next_tuple_ts += _header._freq;
      } while(next_tuple_ts <= ts);

      // done
      return tuple;
  } else if(_header._pos_ts - _header._duration <= ts) {
      // we are somewhere behind but not too much, just update the same way
      out = in;

      // find the spot
      boost::uint32_t pos = _header._pos;
      rrdb_metric_tuple_t * tuple = &_tuples[pos];
      boost::int64_t tuple_ts = tuple->_ts;
      do {
          // move pos
          _header._pos = this->get_prev_pos(_header._pos);
          tuple = &_tuples[pos];

          tuple_ts -= _header._freq;
          tuple->_ts = tuple_ts; // overwrite just in case (it might not be initialized!)
      } while(ts < tuple_ts);

      return tuple;
  } else {
      // we are WAY behind, ignore but let the next block try
      out = in;

      // done
      CHECK_AND_LOG2(ts <_header._pos_ts - _header._duration, NULL);
      return NULL;
  }
}

void rrdb_metric_block::update(const update_ctx_t & in, update_ctx_t & out)
{
  rrdb_metric_tuple_t * tuple = this->find_tuple(in, out);
  if(!tuple) {
      log::write(log::LEVEL_DEBUG, "Can not find tuple for timestamp %llu", in.get_ts());
      return;
  }
  CHECK_AND_LOG(tuple->_ts <= in.get_ts() && in.get_ts() < tuple->_ts + _header._freq);

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
    throw new exception("Unexpected update ctx state %d", in._state);
  }
}

bool rrdb_metric_block::select(rrdb_metric_block::select_ctx & ctx) const
{
  CHECK_AND_LOG2(_tuples.get(), false);
  CHECK_AND_LOG2(_header._pos < _header._count, false);

  // log::write(log::LEVEL_DEBUG, "Select from %lld to %lld (we have %lld to %lld)", ctx._ts_begin, ctx._ts_end, this->get_earliest_ts(), this->get_latest_ts());

  if(ctx._ts_end < this->get_earliest_ts()) {
      // try earlier blocks
      return true;
  }
  if(this->get_latest_ts() < ctx._ts_begin) {
      // no point, stop
      return false;
  }

  boost::uint32_t pos(_header._pos);
  do {
      const rrdb_metric_tuple_t & tuple = _tuples[pos];
      if(tuple._ts < ctx._ts_begin) {
          return false;
      }
      if(tuple._ts < ctx._ts_end) {
          ctx._ts_end = tuple._ts;
          ctx.append(tuple, _header._freq);
      }

      // move to prev one
      pos = this->get_prev_pos(pos);
  } while(pos != _header._pos);

  // try next block
  return true;
}

void rrdb_metric_block::write_block(std::fstream & ofs)
{
  CHECK_AND_LOG(_tuples.get());

  // write header
  ofs.write((const char*)&_header, sizeof(_header));

  // write data
  ofs.write((const char*)_tuples.get(), _header._data_size);
}

void rrdb_metric_block::read_block(std::fstream & ifs)
{
  CHECK_AND_LOG(!_tuples.get());

  // rememeber where are we
  uint64_t offset = ifs.tellg();

  // read header
  ifs.read((char*)&_header, sizeof(_header));
  if(_header._magic != RRDB_METRIC_BLOCK_MAGIC) {
      throw exception("Unexpected rrdb metric block magic: %04x", _header._magic);
  }
  if(_header._freq <= 0) {
      throw exception("Unexpected rrdb metric block frequency: %llu", _header._freq);
  }
  if(_header._count <= 0) {
      throw exception("Unexpected rrdb metric block count: %llu", _header._count);
  }
  if(_header._duration != _header._freq * _header._count) {
      throw exception("Unexpected rrdb metric block duration: %llu (expected %llu)", _header._duration, _header._freq * _header._count);
  }
  if(_header._pos >= _header._count) {
      throw exception("Unexpected rrdb metric block pos: %llu", _header._pos);
  }
  if(_header._offset != offset) {
      throw exception("Unexpected rrdb metric block offset: %llu (expected %llu)", _header._offset, offset);
  }
  if(_header._data_size != _header._count * sizeof(rrdb_metric_tuple_t)) {
      throw exception("Unexpected rrdb metric block data size: %llu (expected %llu)", _header._data_size, _header._count * sizeof(rrdb_metric_tuple_t));
  }

  // read data
  _tuples.reset(new rrdb_metric_tuple_t[_header._count]);
  ifs.read((char*)_tuples.get(), _header._data_size);

  if(_tuples[_header._pos]._ts != _header._pos_ts) {
      throw exception("Unexpected rrdb metric block pos %d pos_ts: %llu (expected from tuple: %llu)",  _header._pos, _header._pos_ts, _tuples[_header._pos]._ts);
  }
}
