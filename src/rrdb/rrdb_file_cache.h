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
#include "spinlock.h"

class rrdb;
class config;
class rrdb_file_cache_impl;

class rrdb_file_cache
{
public:
  typedef boost::shared_ptr<std::fstream> fstream_ptr;

public:
  rrdb_file_cache();
  virtual ~rrdb_file_cache();

  void initialize(boost::shared_ptr<config> config);
  void clear_cache();

  my::size_t get_cache_size() const;
  my::size_t get_cache_hits() const;
  my::size_t get_cache_misses() const;

  fstream_ptr open_file(const std::string & filename, bool new_file = false);
  void move_file(const std::string & from, const std::string & to);
  void delete_file(const std::string & filename);

  std::string get_filename(const std::string & metric_name) const;
  void load_metrics(const boost::shared_ptr<rrdb> & rrdb, rrdb::t_metrics_map & metrics);

private:
  std::string get_full_path(const std::string & filename) const;

private:
  mutable spinlock                        _lock;
  std::string                             _path;
  boost::shared_ptr<rrdb_file_cache_impl> _files_cache_impl;
}; // rrdb_file_cache

#endif /* RRDB_FILE_CACHE_H_ */
