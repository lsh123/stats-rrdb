/*
 * rrdb_metric.cpp
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */
#include <stdio.h>

#include <boost/thread/locks.hpp>
#include <boost/functional/hash.hpp>
#include <boost/foreach.hpp>

#include "parser/statements.h"

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_file_cache.h"
#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_metric_block.h"

#include "log.h"
#include "exception.h"

#define RRDB_METRIC_MAGIC               0xDB99
#define RRDB_METRIC_VERSION             0x01



rrdb_metric::rrdb_metric(const std::string & filename) :
  _filename(filename)
{
  // setup empty header
  memset(&_header, 0, sizeof(_header));
  _header._magic        = RRDB_METRIC_MAGIC;
  _header._version      = RRDB_METRIC_VERSION;
}

rrdb_metric::~rrdb_metric()
{
}

// use copy here to avoid problems with MT
std::string rrdb_metric::get_name()
{
  boost::lock_guard<spinlock> guard(_lock);
  return std::string(_name.get(), _header._name_len);
}

// use copy here to avoid problems with MT
retention_policy rrdb_metric::get_policy()
{
  boost::lock_guard<spinlock> guard(_lock);
  CHECK_AND_THROW(_blocks.size() == _header._blocks_size);

  retention_policy res;
  res.reserve(_header._blocks_size);
  BOOST_FOREACH(const rrdb_metric_block & block, _blocks) {
    retention_policy_elem elem;
    elem._freq     = block.get_freq();
    elem._duration = block.get_freq() * block.get_count();

    res.push_back(elem);
  }
  return res;
}


void rrdb_metric::set_name_and_policy(const std::string & filename, const std::string & name, const retention_policy & policy)
{
  {
    boost::lock_guard<spinlock> guard(_lock);

    // copy name
    _header._name_len   = name.length();
    _header._name_size  = rrdb_metric::get_padded_name_len(_header._name_len);
    _name.reset(new char[_header._name_size]);
    memset(_name.get(), 0, _header._name_size);
    std::copy(name.begin(), name.end(), _name.get());

    // copy policy
    _header._blocks_size  = policy.size();
    _blocks.clear();
    _blocks.reserve(_header._blocks_size);
    my::size_t offset = sizeof(_header) + _header._name_size;
    BOOST_FOREACH(const retention_policy_elem & elem, policy) {
      _blocks.push_back(rrdb_metric_block(elem._freq, elem._duration / elem._freq, offset));
      offset += _blocks.back().get_size();
    }

    // set filename
    _filename = filename;
  }
}

bool rrdb_metric::is_dirty()
{
  boost::lock_guard<spinlock> guard(_lock);
  return my::bitmask_check<boost::uint16_t>(_header._status, Status_Dirty);
}

bool rrdb_metric::is_deleted()
{
  boost::lock_guard<spinlock> guard(_lock);
  return my::bitmask_check<boost::uint16_t>(_header._status, Status_Deleted);
}

void rrdb_metric::get_last_value(my::value_t & value, my::time_t & value_ts)
{
  boost::lock_guard<spinlock> guard(_lock);
  value    = _header._last_value;
  value_ts = _header._last_value_ts;
}

void rrdb_metric::update(const rrdb * const rrdb, const my::time_t & ts, const my::value_t & value)
{
  CHECK_AND_THROW(rrdb);

  boost::lock_guard<spinlock> guard(_lock);
  if(_header._blocks_size <= 0) {
      return;
  }
  CHECK_AND_THROW(_blocks.size() == _header._blocks_size);

  LOG(log::LEVEL_DEBUG2, "Update '%s' with %f at timestamp %ld", _name.get(), value, ts);

  // mark dirty
  my::bitmask_set<boost::uint16_t>(_header._status, Status_Dirty);

  // update last value if new is greater
  if(_header._last_value_ts <= ts) {
      _header._last_value_ts = ts;
      _header._last_value    = value;
  }

  //
  // The update can have 3 possible effects:
  //
  // - We update "current" in the block - we need to do nothing else at this point
  //   since the "next" block doesn't have the data yet
  //
  // - We have to update something in the past (history) - need to go to the next block
  //
  // - We are moving past the current in this block - roll up in the next block
  //
  rrdb_metric_block::update_ctx_t one, two;
  one._state = rrdb_metric_block::UpdateState_Value;
  one._ts    = ts;
  one._value = value;
  my::size_t ii(1); // start from block 1
  BOOST_FOREACH(rrdb_metric_block & block, _blocks) {
      // swap one and two to avoid copying data
      if(ii == 1) {
          LOG(log::LEVEL_DEBUG3, "Updating block with 'one' at ts %lld with ctx state %d", one.get_ts(), one._state);

          block.update(rrdb, this, one, two);
          if(two._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }
          ii = 2; // next block 2
      } else {
          LOG(log::LEVEL_DEBUG3, "Updating block with 'two' at ts %lld with ctx state %d", two.get_ts(), two._state);

          block.update(rrdb, this, two, one);
          if(one._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }

          ii = 1; // next block 1
      }
  }
}

void rrdb_metric::select(const rrdb * const rrdb, const my::time_t & ts1, const my::time_t & ts2, rrdb::data_walker & walker)
{
  CHECK_AND_THROW(rrdb);

  boost::lock_guard<spinlock> guard(_lock);
  if(_header._blocks_size <= 0) {
      return;
  }

  // note that logic for checking timestamps in rrdb_metric_block::select()
  // is very similar
  BOOST_FOREACH(const rrdb_metric_block & block, _blocks) {
    int res = my::interval_overlap(block.get_earliest_ts(), block.get_latest_ts(), ts1, ts2);
    if(res < 0) {
        // [block) < [ts1, ts2): blocks are ordered from newest to oldest, so we
        // are done - all the next blocks will be earlier than this one
        break;
    }
    if(res ==  0) {
        // res == 0 => block and interval intersect!
        block.select(rrdb, this, ts1, ts2, walker);
    }
  }

  // done - finish up
  walker.flush();
}

// align by 64 bits = 8 bytes
my::size_t rrdb_metric::get_padded_name_len(const my::size_t & name_len)
{
  return name_len + (8 - (name_len % 8));
}


std::string rrdb_metric::get_filename()
{
  boost::lock_guard<spinlock> guard(_lock);
  return _filename;
}

void rrdb_metric::save_file(const rrdb * const rrdb)
{
  CHECK_AND_THROW(rrdb);

  // check if deleted meantime
  if(this->is_deleted()) {
      return;
  }

  // open file
  std::string filename = this->get_filename();
  std::string filename_tmp = filename + ".tmp";
  LOG(log::LEVEL_DEBUG, "RRDB metric '%s' saving file '%s'", this->get_name().c_str(), filename_tmp.c_str());

  boost::shared_ptr<std::fstream> ofs(rrdb->get_file_cache()->open_file(filename_tmp, true));

  // write data (under lock!
  {
    boost::lock_guard<spinlock> guard(_lock);
    this->write_header(*ofs);
    BOOST_FOREACH(rrdb_metric_block & block, _blocks) {
      block.write_block(rrdb, this, *ofs);
    }

    // not dirty!
    my::bitmask_clear<boost::uint16_t>(_header._status, Status_Dirty);
  }

  // flush and  close
  ofs->flush();
  ofs->close();

  // move file
  rrdb->get_file_cache()->move_file(filename_tmp, filename);

  // done
  LOG(log::LEVEL_DEBUG2, "RRDB metric '%s' saved file '%s'", this->get_name().c_str(), filename.c_str());

  // check if deleted meantime
  if(this->is_deleted()) {
      this->delete_file(rrdb);
  }
}

void rrdb_metric::load_file(const rrdb * const rrdb)
{
  CHECK_AND_THROW(rrdb);

  // open file
  std::string filename = this->get_filename();
  LOG(log::LEVEL_DEBUG, "RRDB metric loading file '%s'", filename.c_str());

  boost::shared_ptr<std::fstream> ifs(rrdb->get_file_cache()->open_file(filename));

  // read data under lock
  {
    boost::lock_guard<spinlock> guard(_lock);
    this->read_header(*ifs);
    this->_blocks.reserve(this->_header._blocks_size);
    for(my::size_t ii = 0; ii < this->_header._blocks_size; ++ii) {
        this->_blocks.push_back(rrdb_metric_block());
        this->_blocks.back().read_block(rrdb, this, *ifs);
    }
  }
  ifs->close();

  // done
  LOG(log::LEVEL_DEBUG2, "RRDB metric '%s'  loaded from file '%s'", this->get_name().c_str(), filename.c_str());
}

void rrdb_metric::delete_file(const rrdb * const rrdb)
{
  CHECK_AND_THROW(rrdb);

  // mark as deleted in case the flush thread picks it up in the meantime
  {
    boost::lock_guard<spinlock> guard(_lock);
    my::bitmask_set<boost::uint16_t>(_header._status, Status_Deleted);
  }

  // start
  LOG(log::LEVEL_DEBUG, "RRDB metric '%s' deleting file '%s", this->get_name().c_str(), this->get_filename().c_str());

  rrdb->get_file_cache()->delete_file(this->get_filename());

  // done
  LOG(log::LEVEL_DEBUG2, "RRDB metric '%s' deleted file '%s'", this->get_name().c_str(), this->get_filename().c_str());
}

void rrdb_metric::write_header(std::fstream & ofs)
{
  // should be locked
  CHECK_AND_THROW(_lock.is_locked());

  // write header
  ofs.write((const char*)&_header, sizeof(_header));

  // write name
  ofs.write((const char*)_name.get(), _header._name_size);

  LOG(log::LEVEL_DEBUG, "RRDB metric header: wrote '%s'", _name.get());
}

void rrdb_metric::read_header(std::fstream & ifs)
{
  // should be locked
  CHECK_AND_THROW(_lock.is_locked());

  // read header
  ifs.read((char*)&_header, sizeof(_header));
  if(_header._magic != RRDB_METRIC_MAGIC) {
      throw exception("Unexpected rrdb metric magic: %04x", _header._magic);
  }
  if(_header._version != RRDB_METRIC_VERSION) {
      throw exception("Unexpected rrdb metric version: %04x", _header._version);
  }

  // name
  _name.reset(new char[_header._name_size]);
  ifs.read((char*)_name.get(), _header._name_size);

  LOG(log::LEVEL_DEBUG, "RRDB metric: read '%s'", _name.get());
}
