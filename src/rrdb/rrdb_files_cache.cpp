/*
 * rrdb_files_cache.cpp
 *
 *  Created on: Jun 20, 2013
 *      Author: aleksey
 */

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_files_cache.h"
#include "rrdb/rrdb_metric.h"

#include "common/lru_cache.h"
#include "common/config.h"
#include "common/log.h"
#include "common/exception.h"
#include "common/spinlock.h"


// black magic to make forward declarations work
class rrdb_files_cache_impl :
    public lru_cache<std::string, rrdb_files_cache::fstream_ptr, my::time_t>
{
}; // rrdb_files_cache_impl

rrdb_files_cache::rrdb_files_cache():
  _path("/var/lib/rrdb/"),
  _max_size(1024),
  _purge_threshold(0.8),
  _files_cache_impl(new rrdb_files_cache_impl())
{

}

rrdb_files_cache::~rrdb_files_cache()
{
}

void rrdb_files_cache::initialize(boost::shared_ptr<config> config)
{
  CHECK_AND_THROW(config);

  // set path
  this->set_path(config->get<std::string>("rrdb.path", this->get_path()));

  // set cache size
  this->set_max_size(config->get<my::size_t>("rrdb.open_files_cache_size", this->get_max_size()));

  // simple - under lock
  {
    boost::lock_guard<spinlock> guard(_lock);
    _purge_threshold = config->get<double>("rrdb.open_files_cache_purge_threshold", _purge_threshold);
    if(_purge_threshold > 1.0) {
        throw exception("The rrdb.open_files_cache_purge_threshold should not exceed 1.0");
    }
  }

  LOG(log::LEVEL_INFO, "Initialized open files cache: %lu open files with %f purge threshold",
      this->get_max_size(),
      _purge_threshold
  );
}

my::size_t rrdb_files_cache::get_max_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _max_size;
}

void rrdb_files_cache::set_max_size(const my::size_t & max_size)
{
  CHECK_AND_THROW(max_size > 0);

  boost::lock_guard<spinlock> guard(_lock);
  if(_max_size < max_size) {
      _max_size = max_size;
      this->purge();
  } else {
      _max_size = max_size;
  }

  LOG(log::LEVEL_DEBUG, "Set max size to %ld", max_size);
}

std::string rrdb_files_cache::get_path() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _path;
}

void rrdb_files_cache::set_path(const std::string & path)
{
  CHECK_AND_THROW(!path.empty());

  boost::lock_guard<spinlock> guard(_lock);
  _path = path;
  if((*_path.rbegin()) != '/') {
      _path += "/";
  }
  LOG(log::LEVEL_DEBUG, "Using base folder '%s'", _path.c_str());
}


void rrdb_files_cache::clear()
{
  LOG(log::LEVEL_INFO, "Clearing open files cache");

  boost::lock_guard<spinlock> guard(_lock);
  _files_cache_impl->clear();
}

my::size_t rrdb_files_cache::get_cache_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _files_cache_impl->get_size();
}

my::size_t rrdb_files_cache::get_cache_hits(bool reset)
{
  boost::lock_guard<spinlock> guard(_lock);
  my::size_t res(_files_cache_impl->get_cache_hits());
  if(reset) {
      _files_cache_impl->reset_cache_hits();
  }
  return res;
}

my::size_t rrdb_files_cache::get_cache_misses(bool reset)
{
  boost::lock_guard<spinlock> guard(_lock);
  my::size_t res(_files_cache_impl->get_cache_misses());
  if(reset) {
      _files_cache_impl->reset_cache_misses();
  }
  return res;
}

std::string rrdb_files_cache::get_full_path(const std::string & filename) const
{
  CHECK_AND_THROW(!filename.empty());
  boost::lock_guard<spinlock> guard(_lock);
  return _path + filename;
}

std::string rrdb_files_cache::get_full_path(const my::filename_t & filename) const
{
  CHECK_AND_THROW(filename);
  return this->get_full_path(*filename);
}

void rrdb_files_cache::purge()
{
  LOG(log::LEVEL_DEBUG3, "Purging open files cache: %lu files before", _files_cache_impl->get_size());

  // should be locked
  CHECK_AND_THROW(_lock.is_locked());

  rrdb_files_cache_impl::t_lru_iterator it(_files_cache_impl->lru_begin());
  rrdb_files_cache_impl::t_lru_iterator it_end(_files_cache_impl->lru_end());
  my::size_t purge_limit = _max_size * _purge_threshold;
  while(it != it_end && _files_cache_impl->get_size() >= purge_limit) {
      it = _files_cache_impl->lru_erase(it);
  }

  LOG(log::LEVEL_DEBUG3, "Purged open files cache: %lu files after", _files_cache_impl->get_size());
}

rrdb_files_cache::fstream_ptr rrdb_files_cache::open_file(
    const std::string & full_path,
    const std::ios_base::openmode & mode
) {
  LOG(log::LEVEL_DEBUG3, "Opening file '%s'", full_path.c_str());

  try {
      fstream_ptr fs(new std::fstream(full_path.c_str(), std::ios_base::binary | std::ios_base::out | std::ios_base::in | mode));
      fs->exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs
      return fs;
  } catch(const std::exception & e) {
      throw exception("Unable to open file '%s': %s", full_path.c_str(), e.what());
  } catch(...) {
      throw exception("Unable to open file '%s': unknown exception", full_path.c_str());
  }
}

rrdb_files_cache::fstream_ptr rrdb_files_cache::open_file(
    const my::filename_t & filename,
    const my::time_t & ts
)
{
  CHECK_AND_THROW(filename);

  // we break this function into three pieces: find (under lock),
  // open file (no lock) and insert (under lock) to make sure we
  // don't perform IO operations under the lock
  LOG(log::LEVEL_DEBUG3, "Looking for file '%s'", filename->c_str());
  boost::shared_ptr<std::fstream> fs;
  std::string base_folder;

  //
  // Try to find
  //
  {
    boost::lock_guard<spinlock> guard(_lock);
    fs = _files_cache_impl->find(*filename, ts);
    if(fs) {
        return fs;
    }

    // while we are under lock copy base folder...
    base_folder = _path;
  }

  //
  // open a new fstream - OUTSIDE of the lock!
  //
  std::string full_path = base_folder + (*filename);

  LOG(log::LEVEL_DEBUG3, "Opening file cache file '%s', full path '%s'", filename->c_str(), full_path.c_str());
  fs = rrdb_files_cache::open_file(full_path);

  //
  // Insert back into cache - under lock
  //
  {
    boost::lock_guard<spinlock> guard(_lock);

    // purge cache if needed
    if(_files_cache_impl->get_size() >= _max_size) {
        this->purge();
    }

    // put it in the cache
    _files_cache_impl->insert(*filename, fs, ts);
  }

  //
  // done
  //
  return fs;
}

void rrdb_files_cache::delete_file(const my::filename_t & filename)
{
  CHECK_AND_THROW(filename);

  std::string full_path = this->get_full_path(filename);
  LOG(log::LEVEL_DEBUG3, "Deleting file '%s', full path '%s'", filename->c_str(), full_path.c_str());

  // don't forget to cleanup any open file handles
  fstream_ptr fs;
  {
    boost::lock_guard<spinlock> guard(_lock);
    fs = _files_cache_impl->erase(*filename);
  }

  // close it manually here, if any other thread has a handle then it will
  // get IO failure
  if(fs) {
      fs->close();
      fs.reset();
  }

  // delete
  boost::filesystem::remove(full_path);
}
