/*
 * rrdb_metric_tuples_cache.cpp
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */
#include <boost/foreach.hpp>

#include "parser/memory_size.h"

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_metric_block.h"
#include "rrdb/rrdb_metric_tuples_cache.h"

#include "common/lru_cache.h"
#include "common/config.h"
#include "common/log.h"
#include "common/exception.h"


class rrdb_metric_tuples_cache_elem
{
public:
  rrdb_metric_tuples_cache_elem(
    const rrdb_metric_tuples_t & tuples = rrdb_metric_tuples_t(),
    const my::memory_size_t & tuples_size  = 0
  ) :
    _tuples(tuples),
    _tuples_size(tuples_size)
  {
  }

  bool is_valid() const
  {
    return _tuples && _tuples_size;
  }

public:
  rrdb_metric_tuples_t _tuples;
  my::memory_size_t    _tuples_size;
}; // rrdb_metric_tuples_cache_elem

// black magic to make forward declarations work
class rrdb_metric_tuples_cache_impl :
    public lru_cache<
      const rrdb_metric_block * const,
      rrdb_metric_tuples_cache_elem,
      my::size_t
    >
{
}; // rrdb_metric_tuples_cache_impl


rrdb_metric_tuples_cache::rrdb_metric_tuples_cache(
    const boost::shared_ptr<rrdb_files_cache> & files_cache
):
  _max_used_memory(1 * MEMORY_SIZE_GIGABYTE), // 1GB
  _purge_threshold(0.8),
  _used_memory(0),
  _files_cache(files_cache),
  _tuples_cache_impl(new rrdb_metric_tuples_cache_impl())
{
}

rrdb_metric_tuples_cache::~rrdb_metric_tuples_cache()
{
}


void rrdb_metric_tuples_cache::initialize(boost::shared_ptr<config> config)
{
  // set cache size
  // TODO: use memory_use parsers here
  // TODO: put these params into config

  this->set_max_used_memory(memory_size_parse(
      config->get<std::string>("rrdb.blocks_cache_memory_used", memory_size_write(
          this->get_max_used_memory()
      ))
  ));

  // simple - under lock
  {
    boost::lock_guard<spinlock> guard(_lock);
    _purge_threshold = config->get<double>("rrdb.blocks_cache_purge_threshold", _purge_threshold);
    if(_purge_threshold > 1.0) {
        throw exception("The rrdb.open_files_cache_purge_threshold should not exceed 1.0");
    }
  }
}

rrdb_metric_tuples_t rrdb_metric_tuples_cache::find(
    const rrdb_metric_block * const block,
    const my::time_t & ts
) {
  LOG(log::LEVEL_DEBUG3, "Looking for block '%p'", block);

  boost::lock_guard<spinlock> guard(_lock);
  rrdb_metric_tuples_cache_elem elem(_tuples_cache_impl->find(block, ts));
  if(elem.is_valid()) {
      return elem._tuples;
  }

  return rrdb_metric_tuples_t();
}

void rrdb_metric_tuples_cache::insert(
    const rrdb_metric_block * const block,
    const rrdb_metric_tuples_t & tuples,
    const my::memory_size_t & tuples_size,
    const my::time_t & ts
) {
  LOG(log::LEVEL_DEBUG3, "Inserting block '%p'", block);

  boost::lock_guard<spinlock> guard(_lock);

  // purge cache if needed
  if(_used_memory + tuples_size >= _max_used_memory) {
      this->purge();
  }

  // and put it in the cache
  rrdb_metric_tuples_cache_elem elem(tuples, tuples_size);
  if(_tuples_cache_impl->insert(block, elem, ts)) {
      _used_memory += tuples_size;
  }
}


void rrdb_metric_tuples_cache::erase(
    const rrdb_metric_block * const block
) {
  LOG(log::LEVEL_DEBUG3, "Erasing block '%p'", block);

  boost::lock_guard<spinlock> guard(_lock);
  rrdb_metric_tuples_cache_elem elem = _tuples_cache_impl->erase(block);
  if(elem.is_valid()) {
      _used_memory -= elem._tuples_size;
  }
}

void rrdb_metric_tuples_cache::clear()
{
  boost::lock_guard<spinlock> guard(_lock);
  _tuples_cache_impl->clear();
  _used_memory = 0;
}

void rrdb_metric_tuples_cache::purge()
{
  // should be locked
  CHECK_AND_THROW(_lock.is_locked());

  rrdb_metric_tuples_cache_impl::t_lru_iterator it(_tuples_cache_impl->lru_begin());
  rrdb_metric_tuples_cache_impl::t_lru_iterator it_end(_tuples_cache_impl->lru_end());
  my::memory_size_t purge_limit = _max_used_memory * _purge_threshold;
  while(it != it_end && _used_memory >= purge_limit) {
      LOG(log::LEVEL_DEBUG3, "Purging block '%p'", (*it)._k);

      CHECK_AND_THROW(_used_memory >= (*it)._v._tuples_size);
      _used_memory -= (*it)._v._tuples_size;

      // delete
      it = _tuples_cache_impl->lru_erase(it);
  }
}

my::memory_size_t rrdb_metric_tuples_cache::get_max_used_memory() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _max_used_memory;
}

void rrdb_metric_tuples_cache::set_max_used_memory(const my::memory_size_t & max_used_memory)
{
  boost::lock_guard<spinlock> guard(_lock);
  if(_max_used_memory < max_used_memory) {
      _max_used_memory = max_used_memory;
      this->purge();
  } else {
      _max_used_memory = max_used_memory;
  }
}

my::memory_size_t rrdb_metric_tuples_cache::get_cache_used_memory() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _used_memory;
}

my::size_t rrdb_metric_tuples_cache::get_cache_size() const
{
  boost::lock_guard<spinlock> guard(_lock);
  return _tuples_cache_impl->get_size();
}

my::size_t rrdb_metric_tuples_cache::get_cache_hits(bool reset)
{
  boost::lock_guard<spinlock> guard(_lock);
  my::size_t res(_tuples_cache_impl->get_cache_hits());
  if(reset) {
      _tuples_cache_impl->reset_cache_hits();
  }
  return res;
}

my::size_t rrdb_metric_tuples_cache::get_cache_misses(bool reset)
{
  boost::lock_guard<spinlock> guard(_lock);
  my::size_t res(_tuples_cache_impl->get_cache_misses());
  if(reset) {
      _tuples_cache_impl->reset_cache_misses();
  }
  return res;
}
