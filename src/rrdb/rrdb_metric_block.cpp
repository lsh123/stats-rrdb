/*
 * rrdb_metric_block.cpp
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#include "rrdb_metric_block.h"

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

rrdb_metric_tuple_t * rrdb_metric_block::find_tuple(const boost::uint64_t & ts, bool & is_current_block) {
  CHECK_AND_LOG2(_tuples.get(), NULL);
  CHECK_AND_LOG2(_header._pos < _header._count, NULL);
  CHECK_AND_LOG2(_header._freq > 0, NULL);

  // log::write(log::LEVEL_DEBUG, "Find tuple %p: ts: %lld, cur pos: %lu, cur ts: %lld, cur last ts: %lld", this, ts, _header._pos, _header._pos_ts, _header._pos_ts + _header._duration);

  if(_header._pos_ts + _header._duration <= ts) {
      // complete shift forward - start from 0 and assume ts is the latest
      memset(_tuples.get(), 0, _header._data_size);
      rrdb_metric_tuple_t & tuple = _tuples[0];
      _header._pos      = 0;
      _header._pos_ts   = tuple._ts = this->normalize_ts(ts);

      // log::write(log::LEVEL_DEBUG, "Find tuple %p: shift forward, new cur ts: %lld",  this, _header._pos_ts);

      is_current_block = false;
      return &tuple;
  } else if(_header._pos_ts <= ts) {
      // we are somewhere ahead but not too much
      rrdb_metric_tuple_t * tuple = &_tuples[_header._pos];
      boost::uint64_t next_tuple_ts = tuple->_ts + _header._freq;
      if(ts < next_tuple_ts) {
          // log::write(log::LEVEL_DEBUG, "Find tuple %p: current tuple will work fine", this);

          // our current tuple will do!!!
          is_current_block = true;
          return tuple;
      }
      // log::write(log::LEVEL_DEBUG, "Find tuple %p: current tuple will NOT work: pos=%lu, ts=%lld, next tuple ts=%lld", this, _header._pos, ts, next_tuple_ts);

      // we are shifting forward
      is_current_block   = false;
      do {
          // move the pointer
          ++_header._pos;
          if(_header._pos >= _header._count) {
              _header._pos = 0; // wrap
          }
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
      // we are somewhere behind but not too much
      boost::uint32_t pos = _header._pos;
      rrdb_metric_tuple_t * tuple = &_tuples[pos];
      boost::int64_t tuple_ts = tuple->_ts;
      do {
          // move pos
          if(pos > 0) {
              --pos;
          } else {
              pos = _header._count - 1;
          }
          tuple = &_tuples[pos];
          tuple_ts -= _header._freq;
          tuple->_ts = tuple_ts; // overwrite just in case (it might not be initialized!)
      } while(ts < tuple_ts);

      is_current_block = false;
      return tuple;
  }

  // we are WAY behind, ignore
  CHECK_AND_LOG2(ts <_header._pos_ts - _header._duration, NULL);
  is_current_block = false;
  return NULL;
}

bool rrdb_metric_block::update(const boost::uint64_t & ts, const double & value)
{
  bool is_current_block = false;
  rrdb_metric_tuple_t * tuple = this->find_tuple(ts, is_current_block);
  if(!tuple) {
      log::write(log::LEVEL_DEBUG, "Can not find tuple for timestamp %llu", ts);
      return is_current_block;
  }
  CHECK_AND_LOG2(tuple->_ts <= ts && ts < tuple->_ts + _header._freq, false);

  // update our tuple
  if(tuple->_min > value || tuple->_count == 0) {
      tuple->_min = value;
  }
  if(tuple->_max < value || tuple->_count == 0) {
      tuple->_max = value;
  }
  tuple->_sum += value;
  tuple->_sum_sqr += value * value;
  ++tuple->_count;

  // done
  return is_current_block;
}

bool rrdb_metric_block::update(const rrdb_metric_tuple_t & values)
{
  bool shifted = false;
  rrdb_metric_tuple_t * tuple = this->find_tuple(values._ts, shifted);
  if(!tuple) {
      log::write(log::LEVEL_DEBUG, "Can not find tuple for timestamp %llu", values._ts);
      return shifted;
  }
  CHECK_AND_LOG2(tuple->_ts <= values._ts && values._ts < tuple->_ts + _header._freq, false);

  // update our tuple
  if(tuple->_min > values._min || tuple->_count == 0) {
      tuple->_min = values._min;
  }
  if(tuple->_max < values._max || tuple->_count == 0) {
      tuple->_max = values._max;
  }
  tuple->_sum     += values._sum;
  tuple->_sum_sqr += values._sum_sqr;
  tuple->_count   += values._count;

  // done
  return shifted;
}

boost::uint64_t rrdb_metric_block::select(const boost::uint64_t & ts_begin, const boost::uint64_t & ts_end, std::vector<rrdb_metric_tuple_t> & res) const
{
  CHECK_AND_LOG2(_tuples.get(), ts_end);
  CHECK_AND_LOG2(_header._pos < _header._count, NULL);

  if(ts_end < this->get_earliest_ts() || this->get_latest_ts() < ts_begin) {
      return ts_end;
  }

  boost::uint64_t ts(ts_end);
  boost::uint32_t pos(_header._pos);
  do {
      const rrdb_metric_tuple_t & tuple = _tuples[pos];
      if(tuple._ts < ts_begin) {
          break;
      }
      if(tuple._ts < ts_end) {
          ts = tuple._ts;
          res.push_back(tuple);
      }

      // move to prev one
      if(pos > 0) {
          --pos;
      } else {
          pos = _header._count - 1;
      }
  } while(pos != _header._pos);

  // done
  return ts;
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
