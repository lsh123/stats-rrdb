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

#include "lru_cache.h"
#include "config.h"
#include "log.h"
#include "exception.h"

// black magic to make forward declarations work
class rrdb_metric_tuples_cache_impl :
    public lru_cache<
        boost::shared_ptr<rrdb_metric_block>,
        rrdb_metric_tuples_t,
        my::size_t
    >
{
}; // rrdb_metric_tuples_cache_impl


rrdb_metric_tuples_cache::rrdb_metric_tuples_cache():
    _max_size(1024),
    _blocks_cache_impl(new rrdb_metric_tuples_cache_impl()),
    _cache_hits(0),
    _cache_misses(0)
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

rrdb_metric_tuples_t rrdb_metric_tuples_cache::find_or_load_tuples(
    const boost::shared_ptr<rrdb> & rrdb,
    const boost::shared_ptr<rrdb_metric> & rrdb_metric,
    const boost::shared_ptr<rrdb_metric_block> & rrdb_metric_block
)
{
  my::time_t t = time(NULL);

  boost::lock_guard<spinlock> guard(_lock);

  // TODO: break this function into two pieces: find and insert
  // to make sure we don't perform IO operations under the lock
  LOG(log::LEVEL_DEBUG3, "Looking for block '%p'", rrdb_metric_block.get());

  // try to find
  rrdb_metric_tuples_cache_impl::t_iterator it = _blocks_cache_impl->find(rrdb_metric_block);
  if(it != _blocks_cache_impl->end()) {
      ++_cache_hits;
      return _blocks_cache_impl->use(it, t);
  }

  // purge cache if needed
  if(_blocks_cache_impl->size() >= _max_size) {
      this->purge();
  }

  // load block
  rrdb_metric_tuples_t tuples = rrdb_metric->load_single_block(rrdb, rrdb_metric_block);

  // and put it in the cache
  _blocks_cache_impl->insert(rrdb_metric_block, tuples, t);
  ++_cache_misses;

  // done
  return tuples;
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

void rrdb_metric_tuples_cache::clear_cache()
{
  boost::lock_guard<spinlock> guard(_lock);
  _blocks_cache_impl->clear();
  _cache_hits = _cache_misses = 0;
}

void rrdb_metric_tuples_cache::erase(const boost::shared_ptr<rrdb_metric_block> & rrdb_metric_block)
{

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
  return _blocks_cache_impl->size();
}

my::size_t rrdb_metric_tuples_cache::get_cache_hits() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _cache_hits;
}

my::size_t rrdb_metric_tuples_cache::get_cache_misses() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _cache_misses;
}
