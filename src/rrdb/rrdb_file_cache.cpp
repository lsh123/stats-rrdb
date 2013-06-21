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

  // create subfolders
  // ensure folders exist
  char buf[64];
  for(my::size_t ii = 0; ii < RRDB_METRIC_SUBFOLDERS_NUM; ++ii) {
      snprintf(buf, sizeof(buf), "%lu", SIZE_T_CAST ii);
      boost::filesystem::create_directories(_path + buf);
  }
}

std::string rrdb_file_cache::get_filename(const std::string & name) const
{
  // calculate subfolder
  my::size_t name_hash = boost::hash<std::string>()(name) % RRDB_METRIC_SUBFOLDERS_NUM;
  char buf[64];
  snprintf(buf, sizeof(buf), "%lu/", SIZE_T_CAST name_hash);

  // the name should match the "a-zA-Z0-9._-" pattern so we can safely
  // use it as filename
  return buf + name + RRDB_METRIC_EXTENSION;
}

std::string rrdb_file_cache::get_full_path(const std::string & filename) const
{
  return _path + filename;
}

void rrdb_file_cache::load_metrics(const rrdb * const rrdb, rrdb::t_metrics_map & metrics) const
{
  // ensure folders exist
  boost::filesystem::create_directories(_path);

  my::size_t path_len = _path.length();
  for(boost::filesystem::recursive_directory_iterator end, cur(_path); cur != end; ++cur) {
      std::string full_path = (*cur).path().string();
      LOG(log::LEVEL_DEBUG3, "Checking file %s", _path.c_str());

      // we are looking for files
      if((*cur).status().type() != boost::filesystem::regular_file) {
          continue;
      }

      // with specified extension
      if((*cur).path().extension() != RRDB_METRIC_EXTENSION) {
          continue;
      }

      // load metric
      boost::shared_ptr<rrdb_metric> metric(new rrdb_metric(full_path.substr(path_len - 1)));
      metric->load_file(rrdb);

      std::string name(metric->get_name());
      rrdb::t_metrics_map::const_iterator it = metrics.find(name);
      if(it == metrics.end()) {
          // insert loaded metric into map and return
          metrics[name] = metric;
      }
  }
}
