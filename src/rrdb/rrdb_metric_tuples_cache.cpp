/*
 * rrdb_metric_tuples_cache.cpp
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */
#include <boost/foreach.hpp>

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_metric_block.h"
#include "rrdb/rrdb_metric_tuples_cache.h"

#include "common/lru_cache.h"
#include "common/config.h"
#include "common/log.h"
#include "common/exception.h"

// black magic to make forward declarations work
class rrdb_metric_tuples_cache_impl :
    public lru_cache<
      const rrdb_metric_block * const,
      rrdb_metric_tuples_t,
      my::size_t
    >
{
}; // rrdb_metric_tuples_cache_impl


rrdb_metric_tuples_cache::rrdb_metric_tuples_cache(
    const boost::shared_ptr<rrdb_files_cache> & files_cache
):
    _files_cache(files_cache),
    _max_size(1024),
    _tuples_cache_impl(new rrdb_metric_tuples_cache_impl())
{
}

rrdb_metric_tuples_cache::~rrdb_metric_tuples_cache()
{
}


void rrdb_metric_tuples_cache::initialize(boost::shared_ptr<config> config)
{
  // set cache size
  this->set_max_size(config->get<my::size_t>("rrdb.blocks_cache_size", this->get_max_size()));
}

rrdb_metric_tuples_t rrdb_metric_tuples_cache::find(
    const rrdb_metric_block * const block,
    const my::time_t & ts
) {
  LOG(log::LEVEL_DEBUG3, "Looking for block '%p'", block);

  boost::lock_guard<spinlock> guard(_lock);
  return _tuples_cache_impl->find(block, ts);
}

void rrdb_metric_tuples_cache::insert(
    const rrdb_metric_block * const block,
    const rrdb_metric_tuples_t & tuples,
    const my::time_t & ts
) {
  boost::lock_guard<spinlock> guard(_lock);

  // purge cache if needed
  if(_tuples_cache_impl->get_size() >= _max_size) {
      this->purge();
  }

  // and put it in the cache
  _tuples_cache_impl->insert(block, tuples, ts);
}


void rrdb_metric_tuples_cache::erase(
    const rrdb_metric_block * const block
) {
  boost::lock_guard<spinlock> guard(_lock);
  _tuples_cache_impl->erase(block);
}

void rrdb_metric_tuples_cache::clear()
{
  boost::lock_guard<spinlock> guard(_lock);
  _tuples_cache_impl->clear();
}

void rrdb_metric_tuples_cache::purge()
{
  // should be locked
  CHECK_AND_THROW(_lock.is_locked());

  /* TODO: enable purge later
  t_lru_cache::t_lru_iterator it(_cache.lru_begin());
  t_lru_cache::t_lru_iterator it_end(_cache.lru_end());
  while(it != it_end && _cache.size() >= _max_size) {
      // TODO: check that blocks are not dirty
      // TODO: make sure there is no race condition
      // with the update code
      it = _cache.lru_erase(it);
  }
  */
}

my::size_t rrdb_metric_tuples_cache::get_max_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _max_size;
}

void rrdb_metric_tuples_cache::set_max_size(const my::size_t & max_size)
{
  boost::lock_guard<spinlock> guard(_lock);
  if(_max_size < max_size) {
      _max_size = max_size;
      this->purge();
  } else {
      _max_size = max_size;
  }
}

my::size_t rrdb_metric_tuples_cache::get_cache_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _tuples_cache_impl->get_size();
}

my::size_t rrdb_metric_tuples_cache::get_cache_hits() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _tuples_cache_impl->get_cache_hits();
}

my::size_t rrdb_metric_tuples_cache::get_cache_misses() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _tuples_cache_impl->get_cache_misses();
}
