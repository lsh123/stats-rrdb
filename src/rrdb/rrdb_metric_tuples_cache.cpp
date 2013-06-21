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

class rrdb_metric_tuples_cache_impl
{
  typedef lru_cache<
      boost::shared_ptr<rrdb_metric_block>,
      rrdb_metric_tuples_t,
      my::size_t
  > t_lru_cache;

public:
  rrdb_metric_tuples_cache_impl(const my::size_t & max_size) :
    _max_size(max_size),
    _cache_hits(0),
    _cache_misses(0)
  {
  }

  inline const my::size_t & get_max_size() const
  {

    return _max_size;
  }

  inline void set_max_size(const my::size_t & max_size)
  {
    if(_max_size < max_size) {
        _max_size = max_size;
        this->purge();
    } else {
        _max_size = max_size;
    }
  }

  inline my::size_t get_size() const
  {
    return _cache.size();
  }

  inline my::size_t get_cache_hits() const
  {
    return _cache_hits;
  }

  inline my::size_t get_cache_misses() const
  {
    return _cache_misses;
  }

  inline void erase(const boost::shared_ptr<rrdb_metric_block> & block)
  {
    CHECK_AND_THROW(block);
    _cache.erase(block);
  }

  inline void clear()
  {
    _cache.clear();
    _cache_hits = _cache_misses = 0;
  }

  rrdb_metric_tuples_t find_or_load_block(
      const boost::shared_ptr<rrdb_metric_block> & block,
      const my::time_t & t
  )
  {
    CHECK_AND_THROW(block.get());
    LOG(log::LEVEL_DEBUG3, "Looking for block '%p'", block.get());

    // try to find
    t_lru_cache::t_iterator it = _cache.find(block);
    if(it != _cache.end()) {
        ++_cache_hits;
        return _cache.use(it, t);
    }

    // purge cache if needed
    if(_cache.size() >= _max_size) {
        this->purge();
    }

    // load new one
    rrdb_metric_tuples_t tuples;

    // TODO: load the block data

    // put it in the cache
    _cache.insert(block, tuples, t);
    ++_cache_misses;

    // done
    return tuples;
  }

private:
  void purge()
  {
    t_lru_cache::t_lru_iterator it(_cache.lru_begin());
    t_lru_cache::t_lru_iterator it_end(_cache.lru_end());
    while(it != it_end && _cache.size() >= _max_size) {
        // TODO: check that blocks are not dirty
        // TODO: make sure there is no race condition
        // with the update code
        it = _cache.lru_erase(it);
    }
  }

private:
  t_lru_cache _cache;
  my::size_t  _max_size;
  my::size_t  _cache_hits;
  my::size_t  _cache_misses;
}; // rrdb_metric_tuples_cache_impl

rrdb_metric_tuples_cache::rrdb_metric_tuples_cache():
    _blocks_cache_impl(new rrdb_metric_tuples_cache_impl(1024))
{
}

rrdb_metric_tuples_cache::~rrdb_metric_tuples_cache()
{
}


void rrdb_metric_tuples_cache::initialize(boost::shared_ptr<config> config)
{
  boost::lock_guard<spinlock> guard(_lock);
  // set cache
  _blocks_cache_impl->set_max_size(config->get<my::size_t>(
      "rrdb.blocks_cache_size",
      _blocks_cache_impl->get_max_size()
    )
   );
}

void rrdb_metric_tuples_cache::clear_cache()
{
  boost::lock_guard<spinlock> guard(_lock);
  _blocks_cache_impl->clear();
}

my::size_t rrdb_metric_tuples_cache::get_cache_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _blocks_cache_impl->get_size();
}

my::size_t rrdb_metric_tuples_cache::get_cache_hits() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _blocks_cache_impl->get_cache_hits();
}

my::size_t rrdb_metric_tuples_cache::get_cache_misses() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _blocks_cache_impl->get_cache_misses();
}
