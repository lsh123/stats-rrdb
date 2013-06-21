/*
 * rrdb_file_cache.h
 *
 *  Created on: Jun 20, 2013
 *      Author: aleksey
 */

#ifndef RRDB_FILE_CACHE_H_
#define RRDB_FILE_CACHE_H_

#include <string>

#include "rrdb/rrdb.h"

class rrdb;
class config;

class rrdb_file_cache
{
public:
  rrdb_file_cache();
  virtual ~rrdb_file_cache();

  void initialize(boost::shared_ptr<config> config);
  void load_metrics(const rrdb * const rrdb, rrdb::t_metrics_map & metrics) const;

  std::string get_filename(const std::string & name) const;
  std::string get_full_path(const std::string & filename) const;

private:
  std::string  _path;
};

#endif /* RRDB_FILE_CACHE_H_ */
