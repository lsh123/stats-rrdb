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

#include "parser/statements.h"

#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_metric_block.h"
#include "rrdb/rrdb.h"

#include "log.h"
#include "exception.h"

#define RRDB_METRIC_MAGIC               0xDB99
#define RRDB_METRIC_VERSION             0x01

// make it configurable?
#define RRDB_METRIC_SUBFOLDERS_NUM      512

class rrdb_metric_select_ctx : public rrdb_metric_block::select_ctx
{
public:
  rrdb_metric_select_ctx(const statement_select & query) :
    _query(query),
    _cur_interval(0)
  {
    _ts_begin = query._ts_begin;
    _ts_end   = query._ts_end;

    this->reset();
  }

  virtual ~rrdb_metric_select_ctx()
  {

  }

public:
  // rrdb_metric_block::select_ctx
  void append(const rrdb_metric_tuple_t & tuple, const my::interval_t & interval)
  {
    if(_query._group_by == 0) {
        _res.push_back(tuple);
        return;
    }

    // append
    if(_cur_tuple._count == 0) {
        _cur_tuple._ts = tuple._ts;
    }
    rrdb_metric_tuple_update(_cur_tuple, tuple);
    _cur_interval += interval;

    // time to close group by?
    if(_cur_interval >= _query._group_by) {
        this->close_group_by();
        this->reset();
    }
  }

  inline void flush(std::vector<rrdb_metric_tuple_t> & res)
  {
    this->close_group_by();
    res.swap(_res);
  }

private:
  inline void reset()
  {
    memset(&_cur_tuple, 0, sizeof(_cur_tuple));
    _cur_interval = 0;
  }

  inline void close_group_by()
  {
    if(_cur_interval > 0) {
        LOG(log::LEVEL_DEBUG3, "select: cur = %lld, gb=%lld", _cur_interval, _query._group_by ? *_query._group_by : 0);
        rrdb_metric_tuple_normalize(_cur_tuple,  _query._group_by ? (*_query._group_by) / (double)_cur_interval : 0);
        _res.push_back(_cur_tuple);
    }
  }

private:
  const statement_select &         _query;
  std::vector<rrdb_metric_tuple_t> _res;

  rrdb_metric_tuple_t              _cur_tuple;
  my::interval_t                       _cur_interval;
};


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
  CHECK_AND_THROW(_blocks.size() == _header._blocks_size);

  LOG(log::LEVEL_DEBUG2, "Update '%s' with %f at timestamp %ld", _name.get(), value, ts);

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
  std::size_t ii(1); // start from block 1
  BOOST_FOREACH(rrdb_metric_block & block, _blocks) {
      // swap one and two to avoid copying data
      if(ii == 1) {
          LOG(log::LEVEL_DEBUG3, "Updating block with 'one' at ts %lld with ctx state %d", one.get_ts(), one._state);

          block.update(one, two);
          if(two._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }
          ii = 2; // next block 2
      } else {
          LOG(log::LEVEL_DEBUG3, "Updating block with 'two' at ts %lld with ctx state %d", two.get_ts(), two._state);

          block.update(two, one);
          if(one._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }

          ii = 1; // next block 1
      }
  }
}

void rrdb_metric::select(const statement_select & query, std::vector<rrdb_metric_tuple_t> & res)
{
  boost::lock_guard<spinlock> guard(_lock);
  if(_header._blocks_size <= 0) {
      return;
  }

  rrdb_metric_select_ctx ctx(query);
  BOOST_FOREACH(const rrdb_metric_block & block, _blocks) {
    if(!block.select(ctx)) {
        break;
    }
  }

  ctx.flush(res);
}


std::string rrdb_metric::get_full_path(const std::string & folder, const std::string & name)
{
  // calculate subfolder
  std::size_t name_hash = boost::hash<std::string>()(name) % RRDB_METRIC_SUBFOLDERS_NUM;
  char buf[64];
  snprintf(buf, sizeof(buf), "%lu", SIZE_T_CAST name_hash);
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
  LOG(log::LEVEL_DEBUG, "RRDB metric '%s' saving file", this->get_name().c_str());

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
  LOG(log::LEVEL_DEBUG, "RRDB metric '%s' saved file '%s'", this->get_name().c_str(), full_path.c_str());

  // check if deleted meantime
  if(this->is_deleted()) {
      this->delete_file(folder);
  }
}

boost::shared_ptr<rrdb_metric> rrdb_metric::load_file(const std::string & filename)
{
  // start
  LOG(log::LEVEL_DEBUG, "RRDB metric loading file '%s'", filename.c_str());

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
  LOG(log::LEVEL_DEBUG, "RRDB metric loaded file '%s'", filename.c_str());
  return res;
}

void rrdb_metric::delete_file(const std::string & folder)
{
  // mark as deleted in case the flush thread picks it up in the meantime
  this->set_deleted();

  // start
  LOG(log::LEVEL_DEBUG, "RRDB metric '%s' deleting file", this->get_name().c_str());

  std::string full_path = rrdb_metric::get_full_path(folder, this->get_name());
  boost::filesystem::remove(full_path);

  // done
  LOG(log::LEVEL_DEBUG, "RRDB metric '%s' deleted file '%s'", this->get_name().c_str(), full_path.c_str());
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
