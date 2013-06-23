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

  // basic operations
  rrdb_metric_tuples_t find(
      const rrdb_metric_block * const block,
      const my::time_t & ts
  );
  void insert(
      const rrdb_metric_block * const block,
      const rrdb_metric_tuples_t & tuples,
      const my::memory_size_t & tuples_size,
      const my::time_t & ts
  );
  void erase(
      const rrdb_metric_block * const block
  );
  void clear();

  // params
  my::memory_size_t get_max_used_memory() const;
  void set_max_used_memory(const my::memory_size_t & max_used_memory);


  // stats
  my::memory_size_t get_cache_used_memory() const;
  my::size_t get_cache_size() const;
  my::size_t get_cache_hits() const;
  my::size_t get_cache_misses() const;

  // misc
  inline const boost::shared_ptr<rrdb_files_cache> & get_files_cache() const
  {
    return _files_cache;
  }

private:
  void purge();

private:
  mutable spinlock _lock;

  // params
  my::memory_size_t _max_used_memory;
  double            _purge_threshold;

  // data
  my::memory_size_t _used_memory;
  boost::shared_ptr<rrdb_files_cache> _files_cache;
  boost::shared_ptr<rrdb_metric_tuples_cache_impl> _tuples_cache_impl;
}; // rrdb_metric_tuples_cache

#endif /* RRDB_METRIC_TUPLES_CACHE_H_ */
