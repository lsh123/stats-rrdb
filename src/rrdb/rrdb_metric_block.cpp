/*
 * rrdb_metric_block.cpp
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#include "rrdb/rrdb_metric_block.h"

#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_files_cache.h"
#include "rrdb/rrdb_metric_tuples_cache.h"

#include "common/types.h"
#include "common/log.h"
#include "common/exception.h"

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
  _header._data_size = _header._count * sizeof(t_rrdb_metric_tuple);

  if(_header._count && _header._data_size) {
      _modified_tuples.reset(new t_rrdb_metric_tuples(_header._count));
      CHECK_AND_THROW(_modified_tuples);
      CHECK_AND_THROW(_modified_tuples->get());
      CHECK_AND_THROW(_modified_tuples->get_size() == _header._count);
      CHECK_AND_THROW(_modified_tuples->get_memory_size() == _header._data_size);
  }
}

rrdb_metric_block::~rrdb_metric_block()
{
}


t_rrdb_metric_tuples_ptr rrdb_metric_block::get_tuples(
    const my::filename_t & filename,
    const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache
)
{
  CHECK_AND_THROW(tuples_cache);

  // very easy case: if we have _modified_tuples then we MUST use
  // them since cache could have purged the data already
  if(_modified_tuples) {
      // TODO: notify cache about usage
      CHECK_AND_THROW(_modified_tuples->get());
      return _modified_tuples;
  }

  // easy case
  time_t ts(time(NULL));
  t_rrdb_metric_tuples_ptr the_tuples = tuples_cache->find(this, ts);
  if(the_tuples) {
      CHECK_AND_THROW(the_tuples->get());
      return the_tuples;
  }

  // hard case: load data from disk
  const boost::shared_ptr<rrdb_files_cache> & files_cache(tuples_cache->get_files_cache());
  boost::shared_ptr<std::fstream> ifs(files_cache->open_file(filename, ts));
  ifs->seekg(this->get_offset_to_data(), ifs->beg);
  the_tuples = this->read_block_data(*ifs);
  CHECK_AND_THROW(the_tuples);
  CHECK_AND_THROW(the_tuples->get());

  // insert into cache and we are done
  tuples_cache->insert(this, the_tuples, ts);
  return the_tuples;
}

t_rrdb_metric_tuple * rrdb_metric_block::find_tuple(
    t_rrdb_metric_tuples_ptr & the_tuples,
    const t_update_ctx & in,
    t_update_ctx & out
) {
  CHECK_AND_THROW(the_tuples);
  CHECK_AND_THROW(the_tuples->get());
  CHECK_AND_THROW(the_tuples->get_size() == _header._count);
  CHECK_AND_THROW(the_tuples->get_memory_size() == _header._data_size);
  CHECK_AND_THROW(_header._pos < _header._count);
  CHECK_AND_THROW(_header._freq > 0);

  const my::time_t & ts(in.get_ts());
  if(this->get_latest_possible_ts() <= ts) {
      // complete shift forward, notify about rollup
      // copy data here because we are going to destroy these tuples next
      out._state = UpdateState_Tuple;
      out._tuple = *(the_tuples->get_at(_header._pos));

      // destroy tuples and start from 0, we assume this ts is
      // the latest
      the_tuples->zero();
      t_rrdb_metric_tuple * tuple = the_tuples->get_at(0);
      _header._pos      = 0;
      _header._pos_ts   = tuple->_ts = this->normalize_ts(ts);

      // done
      return tuple;
  } else if(this->get_cur_ts() <= ts) {
      // we are somewhere ahead but not too much
      t_rrdb_metric_tuple * tuple = the_tuples->get_at(_header._pos);
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
          tuple = the_tuples->get_at(_header._pos);

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
          t_rrdb_metric_tuple * tuple = the_tuples->get_at(pos);
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
    const my::filename_t & filename,
    const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache,
    const t_update_ctx & in,
    t_update_ctx & out
)
{
  t_rrdb_metric_tuples_ptr the_tuples(this->get_tuples(filename, tuples_cache));
  CHECK_AND_THROW(the_tuples);
  CHECK_AND_THROW(the_tuples->get());
  CHECK_AND_THROW(the_tuples->get_size() == _header._count);
  CHECK_AND_THROW(the_tuples->get_memory_size() == _header._data_size);
  CHECK_AND_THROW(this->get_cur_ts() == (*the_tuples)[_header._pos]._ts);

  t_rrdb_metric_tuple * tuple = this->find_tuple(the_tuples, in, out);
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

  // remember modified tuples
  _modified_tuples.swap(the_tuples);
}

void rrdb_metric_block::select(
    const my::filename_t & filename,
    const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache,
    const my::time_t & ts1,
    const my::time_t & ts2,
    rrdb::data_walker & walker
)
{
  CHECK_AND_THROW(_header._pos < _header._count);

  t_rrdb_metric_tuples_ptr the_tuples(this->get_tuples(filename, tuples_cache));
  CHECK_AND_THROW(the_tuples);
  CHECK_AND_THROW(the_tuples->get());
  CHECK_AND_THROW(the_tuples->get_size() == _header._count);
  CHECK_AND_THROW(the_tuples->get_memory_size() == _header._data_size);


  // walk through all the tuples until we hit the end or the time stops
  // note that logic for checking timestamps in rrdb_metric::select()
  // is very similar
  rrdb_metric_block_pos_t pos(_header._pos);
  do {
      const t_rrdb_metric_tuple & tuple((*the_tuples)[pos]);
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

my::size_t rrdb_metric_block::write_block(std::ostream & os)
{
  CHECK_AND_THROW(_modified_tuples);
  CHECK_AND_THROW(_modified_tuples->get());
  CHECK_AND_THROW(_modified_tuples->get_size() == _header._count);
  CHECK_AND_THROW(_modified_tuples->get_memory_size() == _header._data_size);

  LOG(log::LEVEL_DEBUG3, "RRDB writing block at offset %ld, size %ld", _header._offset, _header._data_size);
  my::size_t written_bytes(0);

  // write header
  os.write((const char*)&_header, sizeof(_header));
  written_bytes += sizeof(_header);

  // write data
  os.write((const char*)_modified_tuples->get(), _modified_tuples->get_memory_size());
  written_bytes += _modified_tuples->get_memory_size();

  // modified tuples no longer needed
  _modified_tuples.reset();

  // how many bytes we wrote
  return written_bytes;
}

void rrdb_metric_block::read_block(std::istream & is, bool skip_data)
{
  // remember where are we
  my::size_t offset = is.tellg();
  LOG(log::LEVEL_DEBUG3, "RRDB metric block data: reading at pos %lu", offset);

  // read header
  is.read((char*)&_header, sizeof(_header));
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
  if(_header._data_size != _header._count * sizeof(t_rrdb_metric_tuple)) {
      throw exception("Unexpected rrdb metric block data size: %lu (expected %su)", _header._data_size, _header._count * sizeof(t_rrdb_metric_tuple));
  }

  // skip data block - we load it async
  if(skip_data) {
      LOG(log::LEVEL_DEBUG3, "RRDB metric block data: seek %ld", _header._data_size);
      is.seekg(_header._data_size, is.cur);
      _modified_tuples.reset();
  } else {
      _modified_tuples = this->read_block_data(is);
      CHECK_AND_THROW(_modified_tuples);
      CHECK_AND_THROW(_modified_tuples->get());
      CHECK_AND_THROW(_modified_tuples->get_size() == _header._count);
      CHECK_AND_THROW(_modified_tuples->get_memory_size() == _header._data_size);
  }
}

t_rrdb_metric_tuples_ptr rrdb_metric_block::read_block_data(std::istream & is) const
{
  LOG(log::LEVEL_DEBUG3, "RRDB metric block read data at offset %ld, size %ld", _header._offset, _header._data_size);

  // create data
  t_rrdb_metric_tuples_ptr the_tuples(new t_rrdb_metric_tuples(_header._count));
  CHECK_AND_THROW(the_tuples);
  CHECK_AND_THROW(the_tuples->get());
  CHECK_AND_THROW(the_tuples->get_size() == _header._count);
  CHECK_AND_THROW(the_tuples->get_memory_size() == _header._data_size);

  // read data
  is.read((char*)the_tuples->get(), the_tuples->get_memory_size());

  // check data
  if((*the_tuples)[_header._pos]._ts != _header._pos_ts) {
      throw exception("Unexpected rrdb metric block pos %u pos_ts: %ld (expected from tuple: %ld)",  _header._pos, _header._pos_ts, (*the_tuples)[_header._pos]._ts);
  }

  // done
  return the_tuples;
}

