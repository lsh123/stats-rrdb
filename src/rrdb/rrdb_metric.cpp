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
  // TODO: drop blocks from cache?
}

// use copy here to avoid problems with MT
std::string rrdb_metric::get_name() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return std::string(_name.get(), _header._name_len);
}

// use copy here to avoid problems with MT
retention_policy rrdb_metric::get_policy() const
{
  boost::lock_guard<spinlock> guard(_lock);
  CHECK_AND_THROW(_blocks.size() == _header._blocks_size);

  retention_policy res;
  res.reserve(_header._blocks_size);
  BOOST_FOREACH(const boost::shared_ptr<rrdb_metric_block> & block, _blocks) {
    retention_policy_elem elem;
    elem._freq     = block->get_freq();
    elem._duration = block->get_freq() * block->get_count();

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
      boost::shared_ptr<rrdb_metric_block> block(
          new rrdb_metric_block(elem._freq, elem._duration / elem._freq, offset)
      );
      _blocks.push_back(block);
      offset += block->get_size();
    }

    // set filename
    _filename = filename;
  }
}

void rrdb_metric::get_last_value(my::value_t & value, my::time_t & value_ts) const
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
  BOOST_FOREACH(boost::shared_ptr<rrdb_metric_block> & block, _blocks) {
      // swap one and two to avoid copying data
      if(ii == 1) {
          LOG(log::LEVEL_DEBUG3, "Updating block with 'one' at ts %lld with ctx state %d", one.get_ts(), one._state);

          block->update(rrdb, this, one, two);
          if(two._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }
          ii = 2; // next block 2
      } else {
          LOG(log::LEVEL_DEBUG3, "Updating block with 'two' at ts %lld with ctx state %d", two.get_ts(), two._state);

          block->update(rrdb, this, two, one);
          if(one._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }

          ii = 1; // next block 1
      }
  }
}

void rrdb_metric::select(const rrdb * const rrdb, const my::time_t & ts1, const my::time_t & ts2, rrdb::data_walker & walker) const
{
  CHECK_AND_THROW(rrdb);

  boost::lock_guard<spinlock> guard(_lock);
  if(_header._blocks_size <= 0) {
      return;
  }

  // note that logic for checking timestamps in rrdb_metric_block::select()
  // is very similar
  BOOST_FOREACH(const boost::shared_ptr<rrdb_metric_block> & block, _blocks) {
    int res = my::interval_overlap(block->get_earliest_ts(), block->get_latest_ts(), ts1, ts2);
    if(res < 0) {
        // [block) < [ts1, ts2): blocks are ordered from newest to oldest, so we
        // are done - all the next blocks will be earlier than this one
        break;
    }
    if(res ==  0) {
        // res == 0 => block and interval intersect!
        block->select(rrdb, this, ts1, ts2, walker);
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


std::string rrdb_metric::get_filename() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _filename;
}

void rrdb_metric::load_file(const rrdb * const rrdb)
{
  CHECK_AND_THROW(rrdb);

  // operate on the file under lock: one at a time!
  {
    boost::lock_guard<spinlock> guard(_lock);
    LOG(log::LEVEL_DEBUG, "RRDB metric loading file '%s'", _filename.c_str());

    boost::shared_ptr<std::fstream> ifs(rrdb->get_files_cache()->open_file(_filename));
    ifs->seekg(0, ifs->beg);
    ifs->sync();

    // read header
    this->read_header(*ifs);

    // read blocks
    this->_blocks.reserve(this->_header._blocks_size);
    for(my::size_t ii = 0; ii < this->_header._blocks_size; ++ii) {
        boost::shared_ptr<rrdb_metric_block> block(new rrdb_metric_block());
        block->read_block(rrdb, this, *ifs);

        this->_blocks.push_back(block);
    }

    // done
    LOG(log::LEVEL_DEBUG2, "RRDB metric loaded from file '%s'", _filename.c_str());
  }
}

void rrdb_metric::save_file(const rrdb * const rrdb)
{
  CHECK_AND_THROW(rrdb);

  // operate on the file under lock: one at a time!
  {
    boost::lock_guard<spinlock> guard(_lock);
    LOG(log::LEVEL_DEBUG, "RRDB metric saving file '%s'", _filename.c_str());

    // check if file was deleted?
    if(my::bitmask_check<boost::uint16_t>(_header._status, Status_Deleted)) {
        return;
    }

    // open file
    boost::shared_ptr<std::fstream> ofs(rrdb->get_files_cache()->open_file(_filename, true));
    ofs->seekg(0, ofs->beg);

    try {
      // not dirty (clear before writing header)
      my::bitmask_clear<boost::uint16_t>(_header._status, Status_Dirty);

      // write header
      this->write_header(*ofs);

      // write data
      BOOST_FOREACH(boost::shared_ptr<rrdb_metric_block> & block, _blocks) {
        block->write_block(rrdb, this, *ofs);
      }

      // flush, don't close
      ofs->flush();
      ofs->sync();
    } catch(...) {
        // well, still dirty
        my::bitmask_set<boost::uint16_t>(_header._status, Status_Dirty);
        throw;
    }

    // done
    LOG(log::LEVEL_DEBUG2, "RRDB metric saved file '%s'", _filename.c_str());
  }
}


void rrdb_metric::save_dirty_blocks(const rrdb * const rrdb)
{
  CHECK_AND_THROW(rrdb);

  // TODO: implement journal file

  // operate on the file under lock: one at a time!
  {
    boost::lock_guard<spinlock> guard(_lock);
    LOG(log::LEVEL_DEBUG, "RRDB metric saving dirty blocks to file '%s'", _filename.c_str());

    // check if file was deleted?
    if(my::bitmask_check<boost::uint16_t>(_header._status, Status_Deleted)) {
        return;
    }

    // open file - we expect the file to exists
    boost::shared_ptr<std::fstream> ofs(rrdb->get_files_cache()->open_file(_filename));
    ofs->seekg(0, ofs->beg);

    try {
      // not dirty (clear before writing header)
      my::bitmask_clear<boost::uint16_t>(_header._status, Status_Dirty);

      // always write header
      this->write_header(*ofs);

      // write data
      BOOST_FOREACH(boost::shared_ptr<rrdb_metric_block> & block, _blocks) {
        if(block->is_dirty()) {
            ofs->seekg(block->get_offset(), ofs->beg);
            block->write_block(rrdb, this, *ofs);
        }
      }

      // flush, don't close
      ofs->flush();
      ofs->sync();
    } catch(...) {
        // well, still dirty
        my::bitmask_set<boost::uint16_t>(_header._status, Status_Dirty);
        throw;
    }

    // done
    LOG(log::LEVEL_DEBUG2, "RRDB metric saved dirty blocks to file '%s'", _filename.c_str());
  }
}

void rrdb_metric::delete_file(const rrdb * const rrdb)
{
  CHECK_AND_THROW(rrdb);

  // mark as deleted in case the flush thread picks it up in the meantime
  {
    boost::lock_guard<spinlock> guard(_lock);
    LOG(log::LEVEL_DEBUG, "RRDB metric deleting file '%s", _filename.c_str());

    // mark as deleted
    my::bitmask_set<boost::uint16_t>(_header._status, Status_Deleted);

    // TODO: drop blocks from cache

    // delete
    rrdb->get_files_cache()->delete_file(_filename);

    // done
    LOG(log::LEVEL_DEBUG2, "RRDB metric deleted file '%s'", _filename.c_str());
  }
}

void rrdb_metric::write_header(std::fstream & ofs) const
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
