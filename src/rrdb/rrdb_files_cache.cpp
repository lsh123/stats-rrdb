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

// make it configurable?
#define RRDB_METRIC_SUBFOLDERS_NUM      512
#define RRDB_METRIC_EXTENSION           ".rrdb"


// black magic to make forward declarations work
class rrdb_files_cache_impl :
    public lru_cache<std::string, rrdb_files_cache::fstream_ptr, my::time_t>
{
}; // rrdb_files_cache_impl

rrdb_files_cache::rrdb_files_cache():
  _path("/var/lib/rrdb/"),
  _max_size(1024),
  _files_cache_impl(new rrdb_files_cache_impl()),
  _cache_hits(0),
  _cache_misses(0)
{

}

rrdb_files_cache::~rrdb_files_cache()
{
}

void rrdb_files_cache::initialize(boost::shared_ptr<config> config)
{
  // set path
  this->set_path(config->get<std::string>("rrdb.path", this->get_path()));

  // set cache size
  this->set_max_size(config->get<my::size_t>("rrdb.open_files_cache_size", this->get_max_size()));

  // create subfolders
  char buf[64];
  for(my::size_t ii = 0; ii < RRDB_METRIC_SUBFOLDERS_NUM; ++ii) {
      snprintf(buf, sizeof(buf), "%lu", SIZE_T_CAST ii);
      boost::filesystem::create_directories(this->get_path() + buf);
  }
}

std::string rrdb_files_cache::get_filename(const std::string & metric_name) const
{
  // calculate subfolder
  my::size_t name_hash = boost::hash<std::string>()(metric_name) % RRDB_METRIC_SUBFOLDERS_NUM;
  char buf[64];
  snprintf(buf, sizeof(buf), "%lu/", SIZE_T_CAST name_hash);

  // the name should match the "a-zA-Z0-9._-" pattern so we can safely
  // use it as filename
  return buf + metric_name + RRDB_METRIC_EXTENSION;
}

void rrdb_files_cache::load_metrics(const boost::shared_ptr<rrdb> & rrdb, rrdb::t_metrics_map & metrics)
{
  // ensure folders exist
  boost::filesystem::create_directories(_path);

  my::size_t path_len = _path.length();
  for(boost::filesystem::recursive_directory_iterator end, cur(_path); cur != end; ++cur) {
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
      boost::shared_ptr<rrdb_metric> metric(new rrdb_metric(full_path.substr(path_len)));
      metric->load_file(rrdb);

      std::string name(metric->get_name());
      rrdb::t_metrics_map::const_iterator it = metrics.find(name);
      if(it == metrics.end()) {
          // insert loaded metric into map and return
          metrics[name] = metric;
      }
  }
}

my::size_t rrdb_files_cache::get_max_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _max_size;
}

void rrdb_files_cache::set_max_size(const my::size_t & max_size)
{
  boost::lock_guard<spinlock> guard(_lock);
  if(_max_size < max_size) {
      _max_size = max_size;
      this->purge();
  } else {
      _max_size = max_size;
  }
}

std::string rrdb_files_cache::get_path() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _path;
}

void rrdb_files_cache::set_path(const std::string & path)
{
  boost::lock_guard<spinlock> guard(_lock);
  _path = path;
  if((*_path.rbegin()) != '/') {
      _path += "/";
  }
  LOG(log::LEVEL_INFO, "Using base folder '%s'", _path.c_str());
}


void rrdb_files_cache::clear_cache()
{
  boost::lock_guard<spinlock> guard(_lock);
  _files_cache_impl->clear();
  _cache_hits = _cache_misses = 0;
}

my::size_t rrdb_files_cache::get_cache_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _files_cache_impl->size();
}

my::size_t rrdb_files_cache::get_cache_hits() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _cache_hits;
}

my::size_t rrdb_files_cache::get_cache_misses() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _cache_misses;
}

std::string rrdb_files_cache::get_full_path(const std::string & filename) const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _path + filename;
}

void rrdb_files_cache::purge()
{
  // should be locked
  CHECK_AND_THROW(_lock.is_locked());

  rrdb_files_cache_impl::t_lru_iterator it(_files_cache_impl->lru_begin());
  rrdb_files_cache_impl::t_lru_iterator it_end(_files_cache_impl->lru_end());
  while(it != it_end && _files_cache_impl->size() >= _max_size) {
      it = _files_cache_impl->lru_erase(it);
  }
}

rrdb_files_cache::fstream_ptr rrdb_files_cache::open_file(const std::string & filename, bool new_file)
{
  // we break this function into three pieces: find (under lock),
  // open file (no lock) and insert (under lock) to make sure we
  // don't perform IO operations under the lock
  LOG(log::LEVEL_DEBUG3, "Looking for file '%s'", filename.c_str());
  my::time_t t = time(NULL);
  std::string base_folder;

  //
  // Try to find - don't bother if this is a new file
  //
  if(!new_file) {
    boost::lock_guard<spinlock> guard(_lock);
    rrdb_files_cache_impl::t_iterator it = _files_cache_impl->find(filename);
    if(it != _files_cache_impl->end()) {
        ++_cache_hits;
        return _files_cache_impl->use(it, t);
    }

    // while we are under lock copy base folder...
    base_folder = _path;
  } else {
    base_folder = this->get_path();
  }

  //
  // open a new fstream - OUTSIDE of the lock!
  //
  std::string full_path = base_folder + filename;
  LOG(log::LEVEL_DEBUG3, "Opening file '%s', full path '%s', new: %s", filename.c_str(), full_path.c_str(), new_file ? "yes" : "no");

  std::ios_base::openmode mode = std::ios_base::binary | std::ios_base::out | std::ios_base::in;
  if(new_file) {
      mode |= std::ios_base::trunc;
  }
  boost::shared_ptr<std::fstream> fs(new std::fstream(full_path.c_str(), mode));
  fs->exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs

  // TODO: remove
  fs->seekg( 0, fs->end );
  my::size_t sz = fs->tellg();
  fs->seekg( 0, fs->beg );
  sz -= fs->tellg();
  LOG(log::LEVEL_DEBUG3, "Opened file '%s', full path '%s', size: %lu", filename.c_str(), full_path.c_str(), sz);

  //
  // Insert back into cache - under lock
  //
  {
    boost::lock_guard<spinlock> guard(_lock);

    // purge cache if needed
    if(_files_cache_impl->size() >= _max_size) {
        this->purge();
    }

    // put it in the cache
    _files_cache_impl->insert(filename, fs, t);
    ++_cache_misses;
  }

  //
  // done
  //
  return fs;
}

void rrdb_files_cache::move_file(const std::string & from, const std::string & to)
{
  std::string from_full_path = this->get_full_path(from);
  std::string to_full_path = this->get_full_path(to);
  LOG(log::LEVEL_DEBUG3, "Moving file '%s', full path '%s' to file '%s', full path '%s'",
      from.c_str(), from_full_path.c_str(),
      to.c_str(), to_full_path.c_str()
    );

  // don't forget to cleanup any open file handles
  {
    boost::lock_guard<spinlock> guard(_lock);
    _files_cache_impl->erase(from);
    _files_cache_impl->erase(to);
  }

  // move file
  boost::filesystem::rename(from_full_path, to_full_path);
}

void rrdb_files_cache::delete_file(const std::string & filename)
{
  std::string full_path = this->get_full_path(filename);
  LOG(log::LEVEL_DEBUG3, "Deleting file '%s', full path '%s'", filename.c_str(), full_path.c_str());

  // don't forget to cleanup any open file handles
  {
    boost::lock_guard<spinlock> guard(_lock);
    _files_cache_impl->erase(filename);
  }

  // delete
  boost::filesystem::remove(full_path);
}
