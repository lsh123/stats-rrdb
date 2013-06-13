/*
 * rrdb_metric.cpp
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */
#include <stdio.h>

#include <boost/thread/locks.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_metric_block.h"
#include "rrdb/rrdb.h"

#include "log.h"
#include "exception.h"

#define RRDB_METRIC_MAGIC               0xDB99
#define RRDB_METRIC_VERSION             0x01

// make it configurable?
#define RRDB_METRIC_SUBFOLDERS_NUM      512

rrdb_metric::rrdb_metric()
{
  // setup empty header
  memset(&_header, 0, sizeof(_header));
  _header._magic        = RRDB_METRIC_MAGIC;
  _header._version      = RRDB_METRIC_VERSION;
}

rrdb_metric::rrdb_metric(const std::string & name, const retention_policy & policy)
{
  // setup empty header
  memset(&_header, 0, sizeof(_header));
  _header._magic      = RRDB_METRIC_MAGIC;
  _header._version    = RRDB_METRIC_VERSION;

  // copy name
  _header._name_len   = name.length();
  _header._name_size  = rrdb_metric::get_padded_name_len(_header._name_len);
  _name.reset(new char[_header._name_size]);
  memset(_name.get(), 0, _header._name_size);
  std::copy(name.begin(), name.end(), _name.get());

  // copy policy
  _header._blocks_size  = policy.size();
  _blocks.reserve(_header._blocks_size);
  boost::uint64_t offset = sizeof(_header) + _header._name_size;
  BOOST_FOREACH(const retention_policy_elem & elem, policy) {
    _blocks.push_back(rrdb_metric_block(elem._freq, elem._duration / elem._freq, offset));
    offset += _blocks.back().get_size();
  }
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
  CHECK_AND_LOG2(_blocks.size() == _header._blocks_size, retention_policy());

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

bool rrdb_metric::is_dirty()
{
  boost::lock_guard<spinlock> guard(_lock);
  return (_header._status & Status_Dirty);
}

void rrdb_metric::set_dirty()
{
  boost::lock_guard<spinlock> guard(_lock);
  _header._status |= Status_Dirty;
}

bool rrdb_metric::is_deleted()
{
  boost::lock_guard<spinlock> guard(_lock);
  return (_header._status & Status_Deleted);
}

void rrdb_metric::set_deleted()
{
  boost::lock_guard<spinlock> guard(_lock);
  _header._status |= Status_Deleted;
}

void rrdb_metric::update(const boost::uint64_t & ts, const double & value)
{
  boost::lock_guard<spinlock> guard(_lock);
  if(_header._blocks_size <= 0) {
      return;
  }
  CHECK_AND_LOG(_blocks.size() == _header._blocks_size);

  // mark dirty
  _header._status |= Status_Dirty;

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
  for(std::size_t ii = 0; ii < _header._blocks_size; ++ii) {
      // swap one and two to avoid copying data
      if(!(ii & 0x01)) {
          log::write(log::LEVEL_DEBUG, "Updating block %llu with 'one' at ts %lld with ctx state %d", ii, one.get_ts(), one._state);

          _blocks[ii].update(one, two);
          if(two._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }
      } else {
          log::write(log::LEVEL_DEBUG, "Updating block %llu with 'two' at ts %lld with ctx state %d", ii, two.get_ts(), two._state);

          _blocks[ii].update(two, one);
          if(one._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }
      }
  }
}

void rrdb_metric::select(const boost::uint64_t & ts_begin, const boost::uint64_t & ts_end, std::vector<rrdb_metric_tuple_t> & res)
{
  boost::lock_guard<spinlock> guard(_lock);
  if(_header._blocks_size <= 0) {
      return;
  }

  boost::uint64_t ts(ts_end);
  BOOST_FOREACH(const rrdb_metric_block & block, _blocks) {
    ts = block.select(ts_begin, ts, res);

    // done?
    if(ts <= ts_begin) break;
  }
}


std::string rrdb_metric::get_full_path(const std::string & folder, const std::string & name)
{
  // calculate subfolder
  std::size_t name_hash = boost::hash<std::string>()(name) % RRDB_METRIC_SUBFOLDERS_NUM;
  char buf[64];
  snprintf(buf, sizeof(buf), "%zu", name_hash);
  std::string subfolder = folder + "/" + buf;

  // ensure folders exist
  boost::filesystem::create_directories(subfolder);

  // the name should match the "a-zA-Z0-9._-" pattern so we can safely
  // use it as filename
  return subfolder + "/" + name + RRDB_METRIC_EXTENSION;
}

// align by 64 bits = 8 bytes
std::size_t rrdb_metric::get_padded_name_len(std::size_t name_len)
{
  return name_len + (8 - (name_len % 8));
}

void rrdb_metric::save_file(const std::string & folder)
{
  // check if deleted meantime
  if(this->is_deleted()) {
      return;
  }

  // start
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' saving file", this->get_name().c_str());

  // open file
  std::string full_path = rrdb_metric::get_full_path(folder, this->get_name());
  std::string full_path_tmp = full_path + ".tmp";
  std::fstream ofs(full_path_tmp.c_str(), std::ios_base::binary | std::ios_base::out);
  ofs.exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs

  // write data (under lock!
  {
    boost::lock_guard<spinlock> guard(_lock);
    this->write_header(ofs);
    BOOST_FOREACH(rrdb_metric_block & block, _blocks) {
      block.write_block(ofs);
    }
  }

  // flush and  close
  ofs.flush();
  ofs.close();

  // move file
  boost::filesystem::rename(full_path_tmp, full_path);

  // done
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' saved file '%s'", this->get_name().c_str(), full_path.c_str());

  // check if deleted meantime
  if(this->is_deleted()) {
      this->delete_file(folder);
  }
}

boost::shared_ptr<rrdb_metric> rrdb_metric::load_file(const std::string & filename)
{
  // start
  log::write(log::LEVEL_DEBUG, "RRDB metric loading file '%s'", filename.c_str());

  // open file
  std::fstream ifs(filename.c_str(), std::ios_base::binary | std::ios_base::in);
  ifs.exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs

  boost::shared_ptr<rrdb_metric> res(new rrdb_metric());
  res->read_header(ifs);
  res->_blocks.resize(res->_header._blocks_size);
  BOOST_FOREACH(rrdb_metric_block & block, res->_blocks) {
    block.read_block(ifs);
  }
  ifs.close();

  // done
  log::write(log::LEVEL_DEBUG, "RRDB metric loaded file '%s'", filename.c_str());
  return res;
}

void rrdb_metric::delete_file(const std::string & folder)
{
  // mark as deleted in case the flush thread picks it up in the meantime
  this->set_deleted();

  // start
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' deleting file", this->get_name().c_str());

  std::string full_path = rrdb_metric::get_full_path(folder, this->get_name());
  boost::filesystem::remove(full_path);

  // done
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' deleted file '%s'", this->get_name().c_str(), full_path.c_str());
}

void rrdb_metric::write_header(std::fstream & ofs)
{
  // should be locked
  CHECK_AND_LOG(_lock.is_locked());

  // write header
  ofs.write((const char*)&_header, sizeof(_header));

  // write name
  ofs.write((const char*)_name.get(), _header._name_size);

  log::write(log::LEVEL_DEBUG, "RRDB metric header: wrote '%s'", _name.get());
}

void rrdb_metric::read_header(std::fstream & ifs)
{
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

  log::write(log::LEVEL_DEBUG, "RRDB metric: read '%s'", _name.get());
}
