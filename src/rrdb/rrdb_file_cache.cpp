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

#include "config.h"
#include "log.h"
#include "exception.h"


// make it configurable?
#define RRDB_METRIC_SUBFOLDERS_NUM      512
#define RRDB_METRIC_EXTENSION    ".rrdb"

rrdb_file_cache::rrdb_file_cache():
  _path("/var/lib/rrdb/")
{

}

rrdb_file_cache::~rrdb_file_cache()
{
  // TODO Auto-generated destructor stub
}

void rrdb_file_cache::initialize(boost::shared_ptr<config> config)
{
  // get path and ensure it ends with '/'
  _path = config->get<std::string>("rrdb.path", _path);
  if((*_path.rbegin()) != '/') {
      _path += "/";
  }
  LOG(log::LEVEL_INFO, "Using base folder '%s'", _path.c_str());

  // create subfolders
  char buf[64];
  for(my::size_t ii = 0; ii < RRDB_METRIC_SUBFOLDERS_NUM; ++ii) {
      snprintf(buf, sizeof(buf), "%lu", SIZE_T_CAST ii);
      boost::filesystem::create_directories(_path + buf);
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

boost::shared_ptr<std::fstream> rrdb_file_cache::open_file(const std::string & filename, bool new_file)
{
  std::string full_path = _path + filename;
  LOG(log::LEVEL_DEBUG3, "Opening file '%s', full path '%s'", filename.c_str(), full_path.c_str());

  std::ios_base::openmode mode = std::ios_base::binary | std::ios_base::out | std::ios_base::in;
  if(new_file) {
      mode |= std::ios_base::trunc;
  }
  boost::shared_ptr<std::fstream> fs(new std::fstream(full_path.c_str(), mode));
  fs->exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs

  return fs;
}

void rrdb_file_cache::move_file(const std::string & from, const std::string & to)
{
  std::string from_full_path = _path + from;
  std::string to_full_path = _path + to;
  LOG(log::LEVEL_DEBUG3, "Moving file '%s', full path '%s' to file '%s', full path '%s'",
      from.c_str(), from_full_path.c_str(),
      to.c_str(), to_full_path.c_str()
    );

  // move file
  boost::filesystem::rename(from_full_path, to_full_path);
}

void rrdb_file_cache::delete_file(const std::string & filename)
{
  std::string full_path = _path + filename;
  LOG(log::LEVEL_DEBUG3, "Deleting file '%s', full path '%s'", filename.c_str(), full_path.c_str());

  boost::filesystem::remove(full_path);
}
