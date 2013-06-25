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
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "parser/statements.h"

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_files_cache.h"
#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_metric_block.h"
#include "rrdb/rrdb_metric_tuple.h"
#include "rrdb/rrdb_metric_tuples_cache.h"


#include "common/log.h"
#include "common/exception.h"

#define RRDB_METRIC_MAGIC               0xDB99
#define RRDB_METRIC_VERSION             0x01


// make it configurable?
#define RRDB_METRIC_SUBFOLDERS_NUM      512
#define RRDB_METRIC_EXTENSION           ".rrdb"


rrdb_metric::rrdb_metric(const my::filename_t & filename) :
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
std::string rrdb_metric::get_name() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return std::string(_name.get(), _header._name_len);
}

// use copy here to avoid problems with MT
t_retention_policy rrdb_metric::get_policy() const
{
  boost::lock_guard<spinlock> guard(_lock);
  CHECK_AND_THROW(_blocks.size() == _header._blocks_size);

  t_retention_policy res;
  res.reserve(_header._blocks_size);
  BOOST_FOREACH(const boost::intrusive_ptr<rrdb_metric_block> & block, _blocks) {
    t_retention_policy_elem elem;
    elem._freq     = block->get_freq();
    elem._duration = block->get_freq() * block->get_count();

    res.push_back(elem);
  }
  return res;
}


void rrdb_metric::create(const std::string & name, const t_retention_policy & policy)
{
  // check the name
  if(!statement_check_metric_name(name, false)) {
      throw exception("Invalid name '%s'", name.c_str());
  }

  // good, let's do it!
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
  BOOST_FOREACH(const t_retention_policy_elem & elem, policy) {
    boost::intrusive_ptr<rrdb_metric_block> block(
        new rrdb_metric_block(elem._freq, elem._duration / elem._freq, offset)
    );
    _blocks.push_back(block);
    offset += block->get_size();
  }

  // set filename
  _filename = rrdb_metric::construct_filename(name);
}

void rrdb_metric::get_last_value(my::value_t & value, my::time_t & value_ts) const
{
  boost::lock_guard<spinlock> guard(_lock);
  value    = _header._last_value;
  value_ts = _header._last_value_ts;
}

void rrdb_metric::update(
    const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache,
    const my::time_t & ts,
    const my::value_t & value
) {
  CHECK_AND_THROW(tuples_cache);

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
  rrdb_metric_block::t_update_ctx one, two;
  one._state = rrdb_metric_block::UpdateState_Value;
  one._ts    = ts;
  one._value = value;
  my::size_t ii(1); // start from block 1
  BOOST_FOREACH(boost::intrusive_ptr<rrdb_metric_block> & block, _blocks) {
      // swap one and two to avoid copying data
      if(ii == 1) {
          LOG(log::LEVEL_DEBUG3, "Updating block with 'one' at ts %lld with ctx state %d", one.get_ts(), one._state);

          block->update(_filename, tuples_cache, one, two);
          if(two._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }
          ii = 2; // next block 2
      } else {
          LOG(log::LEVEL_DEBUG3, "Updating block with 'two' at ts %lld with ctx state %d", two.get_ts(), two._state);

          block->update(_filename, tuples_cache, two, one);
          if(one._state == rrdb_metric_block::UpdateState_Stop) {
              break;
          }

          ii = 1; // next block 1
      }
  }
}

void rrdb_metric::select(
    const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache,
    const my::time_t & ts1,
    const my::time_t & ts2,
    rrdb::data_walker & walker
) {
  CHECK_AND_THROW(tuples_cache);

  boost::lock_guard<spinlock> guard(_lock);
  if(_header._blocks_size <= 0) {
      return;
  }

  // note that logic for checking timestamps in rrdb_metric_block::select()
  // is very similar
  BOOST_FOREACH(const boost::intrusive_ptr<rrdb_metric_block> & block, _blocks) {
    int res = my::interval_overlap(block->get_earliest_ts(), block->get_latest_ts(), ts1, ts2);
    if(res < 0) {
        // [block) < [ts1, ts2): blocks are ordered from newest to oldest, so we
        // are done - all the next blocks will be earlier than this one
        break;
    }
    if(res ==  0) {
        // res == 0 => block and interval intersect!
        block->select(_filename, tuples_cache, ts1, ts2, walker);
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


my::filename_t rrdb_metric::get_filename() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _filename;
}


void rrdb_metric::load_file(const boost::shared_ptr<rrdb_files_cache> & files_cache)
{
  CHECK_AND_THROW(files_cache);

  // open file and seek to the start of the file
  boost::shared_ptr<std::fstream> fs(files_cache->open_file(this->get_filename(), time(NULL)));
  fs->seekg(0, fs->beg);
  fs->sync();

  // TODO: add better error handling to report exact problem with IO operation
  // operate on the file under lock: one at a time!
  {
    boost::lock_guard<spinlock> guard(_lock);
    LOG(log::LEVEL_DEBUG, "RRDB metric loading file '%s'", _filename->c_str());

    // check if file was deleted?
    if(my::bitmask_check<boost::uint16_t>(_header._status, Status_Deleted)) {
        files_cache->delete_file(_filename);
        return;
    }

    // read header
    this->read_header(*fs);

    // read blocks
    this->_blocks.reserve(this->_header._blocks_size);
    for(my::size_t ii = 0; ii < this->_header._blocks_size; ++ii) {
        boost::intrusive_ptr<rrdb_metric_block> block(new rrdb_metric_block());
        block->read_block(*fs);

        this->_blocks.push_back(block);
    }

    // done
    LOG(log::LEVEL_DEBUG2, "RRDB metric loaded from file '%s'", _filename->c_str());
  }
}

void rrdb_metric::save_file(const boost::shared_ptr<rrdb_files_cache> & files_cache)
{
  CHECK_AND_THROW(files_cache);

  // construct the full paths for tmp and real files
  std::string full_path     = files_cache->get_full_path(this->get_filename());
  std::string tmp_full_path = full_path + ".tmp";

  // open tmp file: do NOT use cache here since this is a temp file
  boost::shared_ptr<std::fstream> fs = rrdb_files_cache::open_file(
      tmp_full_path,
      std::ios_base::trunc      // force new file creation
  );

  // TODO: add better error handling to report exact problem with IO operation

  // operate on the file under lock: one at a time!
  {
    boost::lock_guard<spinlock> guard(_lock);
    LOG(log::LEVEL_DEBUG, "RRDB metric saving file '%s'", _filename->c_str());

    // check if file was deleted?
    if(my::bitmask_check<boost::uint16_t>(_header._status, Status_Deleted)) {
        files_cache->delete_file(_filename);
        return;
    }

    try {
      // not dirty (clear before writing header)
      my::bitmask_clear<boost::uint16_t>(_header._status, Status_Dirty);

      // write header
      this->write_header(*fs);

      // write data
      BOOST_FOREACH(boost::intrusive_ptr<rrdb_metric_block> & block, _blocks) {
        block->write_block(*fs);
      }

      // flush, don't close
      fs->flush();
      fs->sync();
    } catch(...) {
        // well, still dirty
        my::bitmask_set<boost::uint16_t>(_header._status, Status_Dirty);
        throw;
    }

    // move file UNDER LOCK and delete the old file first (this will also
    // purge the open file handle from the cache)
    files_cache->delete_file(_filename);
    boost::filesystem::rename(tmp_full_path, full_path);

    // done
    LOG(log::LEVEL_DEBUG2, "RRDB metric saved file '%s'", _filename->c_str());
  }
}


void rrdb_metric::save_dirty_blocks(const boost::shared_ptr<rrdb_files_cache> & files_cache)
{
  CHECK_AND_THROW(files_cache);

  // open file and seek to the start of the file
  boost::shared_ptr<std::fstream> fs(files_cache->open_file(this->get_filename(), time(NULL)));
  fs->seekg(0, fs->beg);
  fs->sync();

  // TODO: add better error handling to report exact problem with IO operation
  // TODO: implement journal file

  // operate on the file under lock: one at a time!
  {
    boost::lock_guard<spinlock> guard(_lock);
    LOG(log::LEVEL_DEBUG, "RRDB metric saving dirty blocks to file '%s'", _filename->c_str());

    // check if file was deleted?
    if(my::bitmask_check<boost::uint16_t>(_header._status, Status_Deleted)) {
        files_cache->delete_file(_filename);
        return;
    }

    try {
      // not dirty (clear before writing header)
      my::bitmask_clear<boost::uint16_t>(_header._status, Status_Dirty);

      // always write header
      this->write_header(*fs);

      // write data
      BOOST_FOREACH(boost::intrusive_ptr<rrdb_metric_block> & block, _blocks) {
        if(block->is_dirty()) {
            fs->seekg(block->get_offset(), fs->beg);
            block->write_block(*fs);
        }
      }

      // flush, don't close
      fs->flush();
      fs->sync();
    } catch(...) {
        // well, still dirty
        my::bitmask_set<boost::uint16_t>(_header._status, Status_Dirty);
        throw;
    }

    // done
    LOG(log::LEVEL_DEBUG2, "RRDB metric saved dirty blocks to file '%s'", _filename->c_str());
  }
}

void rrdb_metric::delete_file(
    const boost::shared_ptr<rrdb_files_cache> & files_cache,
    const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache
)
{
  CHECK_AND_THROW(tuples_cache);
  CHECK_AND_THROW(files_cache);

  // TODO: add better error handling to report exact problem with IO operation

  // mark as deleted in case the flush thread picks it up in the meantime
  {
    boost::lock_guard<spinlock> guard(_lock);
    LOG(log::LEVEL_DEBUG, "RRDB metric deleting file '%s", _filename->c_str());

    // mark as deleted
    my::bitmask_set<boost::uint16_t>(_header._status, Status_Deleted);

    // drop blocks from cache
    BOOST_FOREACH(const boost::intrusive_ptr<rrdb_metric_block> & block, _blocks) {
      tuples_cache->erase(block.get());
    }

    // delete
    files_cache->delete_file(_filename);

    // done
    LOG(log::LEVEL_DEBUG2, "RRDB metric deleted file '%s'", _filename->c_str());
  }
}

void rrdb_metric::write_header(std::fstream & fs) const
{
  // should be locked
  CHECK_AND_THROW(_lock.is_locked());

  // write header
  fs.write((const char*)&_header, sizeof(_header));

  // write name
  fs.write((const char*)_name.get(), _header._name_size);

  LOG(log::LEVEL_DEBUG, "RRDB metric header: wrote '%s'", _name.get());
}

void rrdb_metric::read_header(std::fstream & fs)
{
  // should be locked
  CHECK_AND_THROW(_lock.is_locked());

  // read header
  fs.read((char*)&_header, sizeof(_header));
  if(_header._magic != RRDB_METRIC_MAGIC) {
      throw exception("Unexpected rrdb metric magic: %04x", _header._magic);
  }
  if(_header._version != RRDB_METRIC_VERSION) {
      throw exception("Unexpected rrdb metric version: %04x", _header._version);
  }

  // name
  _name.reset(new char[_header._name_size]);
  fs.read((char*)_name.get(), _header._name_size);

  LOG(log::LEVEL_DEBUG, "RRDB metric: read header '%s'", _name.get());
}


void rrdb_metric::load_metrics(
    const boost::shared_ptr<rrdb_files_cache> & files_cache,
    const std::string & path,
    rrdb::t_metrics_map & metrics
)
{
  // ensure folders exist
  boost::filesystem::create_directories(path);

  // create subfolders for metrics
  char buf[64];
  for(my::size_t ii = 0; ii < RRDB_METRIC_SUBFOLDERS_NUM; ++ii) {
      snprintf(buf, sizeof(buf), "%lu", SIZE_T_CAST ii);
      boost::filesystem::create_directories(path + buf);
  }

  // check all files and folders
  my::size_t path_len = path.length();
  for(boost::filesystem::recursive_directory_iterator end, cur(path); cur != end; ++cur) {
      std::string full_path = (*cur).path().string();
      LOG(log::LEVEL_DEBUG3, "Checking file %s", full_path.c_str());

      // we are looking for files
      if((*cur).status().type() != boost::filesystem::regular_file) {
          continue;
      }

      // with specified extension
      if((*cur).path().extension() != RRDB_METRIC_EXTENSION) {
          continue;
      }

      // load metric
      LOG(log::LEVEL_DEBUG2, "Loading file %s", full_path.c_str());
      my::filename_t filename(new std::string(full_path.substr(path_len)));
      boost::intrusive_ptr<rrdb_metric> metric(new rrdb_metric(filename));
      metric->load_file(files_cache);

      // insert into the output map
      std::string name(metric->get_name());
      rrdb::t_metrics_map::const_iterator it = metrics.find(name);
      if(it == metrics.end()) {
          // insert loaded metric into map and return
          metrics[name] = metric;
      }
  }
}

my::filename_t rrdb_metric::construct_filename(const std::string & metric_name)
{
  // calculate subfolder
  my::size_t name_hash = boost::hash<std::string>()(metric_name) % RRDB_METRIC_SUBFOLDERS_NUM;
  char buf[64];
  snprintf(buf, sizeof(buf), "%lu/", SIZE_T_CAST name_hash);

  // the name should match the "a-zA-Z0-9._-" pattern so we can safely
  // use it as filename
  my::filename_t res(new std::string(buf + metric_name + RRDB_METRIC_EXTENSION));
  return res;
}

std::string rrdb_metric::normalize_name(const std::string & str)
{
  // force lower case for names
  std::string res(str);
  boost::algorithm::trim(res);
  boost::algorithm::to_lower(res);
  return res;
}


