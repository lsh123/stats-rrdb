/*
 * rrdb_file_cache.cpp
 *
 *  Created on: Jun 20, 2013
 *      Author: aleksey
 */

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_file_cache.h"
#include "rrdb/rrdb_metric.h"

#include "lru_cache.h"
#include "config.h"
#include "log.h"
#include "exception.h"
#include "spinlock.h"

// make it configurable?
#define RRDB_METRIC_SUBFOLDERS_NUM      512
#define RRDB_METRIC_EXTENSION    ".rrdb"

class rrdb_file_cache_impl
{
  typedef lru_cache<std::string, rrdb_file_cache::fstream_ptr, my::time_t> t_lru_cache;

public:
  rrdb_file_cache_impl(const my::size_t & max_size) :
    _max_size(max_size),
    _cache_hits(0),
    _cache_misses(0)
  {
  }

  inline const my::size_t & get_max_size() const
  {

    return _max_size;
  }

  inline void set_max_size(const my::size_t & max_size)
  {
    if(_max_size < max_size) {
        _max_size = max_size;
        this->purge();
    } else {
        _max_size = max_size;
    }
  }

  inline my::size_t get_size() const
  {
    return _cache.size();
  }

  inline my::size_t get_cache_hits() const
  {
    return _cache_hits;
  }

  inline my::size_t get_cache_misses() const
  {
    return _cache_misses;
  }

  inline void erase(const std::string & filename)
  {
    _cache.erase(filename);
  }

  inline void clear()
  {
    _cache.clear();
    _cache_hits = _cache_misses = 0;
  }

  rrdb_file_cache::fstream_ptr get_fstream(
      const std::string & base_folder,
      const std::string & filename,
      const my::time_t & t,
      std::ios_base::openmode mode
  )
  {
    LOG(log::LEVEL_DEBUG3, "Looking for file '%s'", filename.c_str());

    // try to find
    t_lru_cache::t_iterator it = _cache.find(filename);
    if(it != _cache.end()) {
        ++_cache_hits;
        return _cache.use(it, t);
    }

    // purge cache if needed
    if(_cache.size() >= _max_size) {
        this->purge();
    }

    // create new one
    std::string full_path = base_folder + filename;
    LOG(log::LEVEL_DEBUG3, "Opening file '%s', full path '%s'", filename.c_str(), full_path.c_str());

    boost::shared_ptr<std::fstream> fs(new std::fstream(full_path.c_str(), mode));
    fs->exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs

    // put it in the cache
    _cache.insert(filename, fs, t);
    ++_cache_misses;

    // done
    return fs;
  }

private:
  void purge()
  {
    t_lru_cache::t_lru_iterator it(_cache.lru_begin());
    t_lru_cache::t_lru_iterator it_end(_cache.lru_end());
    while(it != it_end && _cache.size() >= _max_size) {
        it = _cache.lru_erase(it);
    }
  }

private:
  t_lru_cache _cache;

  my::size_t  _max_size;
  my::size_t  _cache_hits;
  my::size_t  _cache_misses;
}; // rrdb_file_cache_impl


rrdb_file_cache::rrdb_file_cache():
  _path("/var/lib/rrdb/"),
  _files_cache_impl(new rrdb_file_cache_impl(1024))
{

}

rrdb_file_cache::~rrdb_file_cache()
{
  // TODO Auto-generated destructor stub
}

void rrdb_file_cache::initialize(boost::shared_ptr<config> config)
{
  std::string path;
  {
    boost::lock_guard<spinlock> guard(_lock);
    // set cache
    _files_cache_impl->set_max_size(config->get<my::size_t>(
        "rrdb.open_files_cache_size",
        _files_cache_impl->get_max_size()
      )
     );

    // get path and ensure it ends with '/'
    _path = config->get<std::string>("rrdb.path", _path);
    if((*_path.rbegin()) != '/') {
        _path += "/";
    }
    LOG(log::LEVEL_INFO, "Using base folder '%s'", _path.c_str());

    // remember path so we can create subfolders outside of the lock
    // (doesn't really matter during init phase...)
    path = _path;
  }

  // create subfolders
  char buf[64];
  for(my::size_t ii = 0; ii < RRDB_METRIC_SUBFOLDERS_NUM; ++ii) {
      snprintf(buf, sizeof(buf), "%lu", SIZE_T_CAST ii);
      boost::filesystem::create_directories(path + buf);
  }
}

std::string rrdb_file_cache::get_filename(const std::string & metric_name) const
{
  // calculate subfolder
  my::size_t name_hash = boost::hash<std::string>()(metric_name) % RRDB_METRIC_SUBFOLDERS_NUM;
  char buf[64];
  snprintf(buf, sizeof(buf), "%lu/", SIZE_T_CAST name_hash);

  // the name should match the "a-zA-Z0-9._-" pattern so we can safely
  // use it as filename
  return buf + metric_name + RRDB_METRIC_EXTENSION;
}

void rrdb_file_cache::load_metrics(const rrdb * const rrdb, rrdb::t_metrics_map & metrics)
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


void rrdb_file_cache::clear_cache()
{
  boost::lock_guard<spinlock> guard(_lock);
  _files_cache_impl->clear();
}

my::size_t rrdb_file_cache::get_cache_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _files_cache_impl->get_size();
}

my::size_t rrdb_file_cache::get_cache_hits() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _files_cache_impl->get_cache_hits();
}

my::size_t rrdb_file_cache::get_cache_misses() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _files_cache_impl->get_cache_misses();
}

std::string rrdb_file_cache::get_full_path(const std::string & filename) const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _path + filename;
}

rrdb_file_cache::fstream_ptr rrdb_file_cache::open_file(const std::string & filename, bool new_file)
{
  // if it's a new file, just
  std::ios_base::openmode mode = std::ios_base::binary | std::ios_base::out | std::ios_base::in;
  if(new_file) {
      mode |= std::ios_base::trunc;
  }

  boost::lock_guard<spinlock> guard(_lock);
  return _files_cache_impl->get_fstream(_path, filename, time(NULL), mode);
}

void rrdb_file_cache::move_file(const std::string & from, const std::string & to)
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

void rrdb_file_cache::delete_file(const std::string & filename)
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
