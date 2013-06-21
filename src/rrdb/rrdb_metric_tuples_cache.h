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

#include "types.h"
#include "spinlock.h"

class config;
class rrdb_metric_tuples_cache_impl;


class rrdb_metric_tuples_cache
{
public:
  rrdb_metric_tuples_cache();
  virtual ~rrdb_metric_tuples_cache();

  void initialize(boost::shared_ptr<config> config);
  void clear_cache();

  my::size_t get_cache_size() const;
  my::size_t get_cache_hits() const;
  my::size_t get_cache_misses() const;

private:
  mutable spinlock _lock;
  boost::shared_ptr<rrdb_metric_tuples_cache_impl> _blocks_cache_impl;
}; // rrdb_metric_tuples_cache

#endif /* RRDB_METRIC_TUPLES_CACHE_H_ */
