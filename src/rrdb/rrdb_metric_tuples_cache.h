/*
 * rrdb_metric_tuples_cache.h
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_TUPLES_CACHE_H_
#define RRDB_METRIC_TUPLES_CACHE_H_

#include <boost/shared_ptr.hpp>

#include "rrdb/rrdb_metric_tuple.h"

#include "common/types.h"
#include "common/spinlock.h"

class config;
class rrdb_files_cache;
class rrdb_metric_tuples_cache_impl;


class rrdb_metric_tuples_cache
{
public:
  rrdb_metric_tuples_cache(const boost::shared_ptr<rrdb_files_cache> & files_cache);
  virtual ~rrdb_metric_tuples_cache();

  void initialize(boost::shared_ptr<config> config);
  void clear_cache();

  my::size_t get_max_size() const;
  void set_max_size(const my::size_t & max_size);

  rrdb_metric_tuples_t find_or_load_tuples(
      const boost::shared_ptr<rrdb_metric> & rrdb_metric,
      const boost::shared_ptr<rrdb_metric_block> & rrdb_metric_block
  );

  void erase(const boost::shared_ptr<rrdb_metric_block> & rrdb_metric_block);

  my::size_t get_cache_size() const;
  my::size_t get_cache_hits() const;
  my::size_t get_cache_misses() const;

private:
  void purge();

private:
  mutable spinlock _lock;

  // params
  boost::shared_ptr<rrdb_files_cache> _files_cache;
  my::size_t  _max_size;

  // data
  boost::shared_ptr<rrdb_metric_tuples_cache_impl> _tuples_cache_impl;

  // stats
  my::size_t  _cache_hits;
  my::size_t  _cache_misses;
}; // rrdb_metric_tuples_cache

#endif /* RRDB_METRIC_TUPLES_CACHE_H_ */
