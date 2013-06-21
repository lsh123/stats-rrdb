/*
 * rrdb_file_cache.h
 *
 *  Created on: Jun 20, 2013
 *      Author: aleksey
 */

#ifndef RRDB_FILE_CACHE_H_
#define RRDB_FILE_CACHE_H_

#include <string>
#include <fstream>

#include <boost/shared_ptr.hpp>

#include "rrdb/rrdb.h"

#include "types.h"

class rrdb;
class config;
class rrdb_open_files_cache;

class rrdb_file_cache
{
public:
  typedef boost::shared_ptr<std::fstream> fstream_ptr;

public:
  rrdb_file_cache();
  virtual ~rrdb_file_cache();

  void initialize(boost::shared_ptr<config> config);
  void load_metrics(const rrdb * const rrdb, rrdb::t_metrics_map & metrics);

  void clear_cache();
  my::size_t get_cache_size() const;

  fstream_ptr open_file(const std::string & filename, bool new_file = false);
  void move_file(const std::string & from, const std::string & to);
  void delete_file(const std::string & filename);

  std::string get_filename(const std::string & metric_name) const;

private:
  std::string                              _path;
  boost::shared_ptr<rrdb_open_files_cache> _open_files_cache;
};

#endif /* RRDB_FILE_CACHE_H_ */
