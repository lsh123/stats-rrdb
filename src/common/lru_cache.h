/*
 * lru_cache.h
 *
 *  Created on: Jun 20, 2013
 *      Author: aleksey
 */

#ifndef COMMON_LRU_CACHE_H_
#define COMMON_LRU_CACHE_H_

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

#include "common/types.h"

using namespace ::boost::multi_index;

template<
  typename K,           // key
  typename V,           // value
  typename T            // timestamp
>
class lru_cache {

public:
  // value type for the multi-index
  class value_type
  {
  public:
    value_type(const K & k, const V & v, const T & t) : _k(k), _v(v), _t(t) {}

  public:
    K _k;
    V _v;
    T _t;
  }; // value_type

private:
  class change_time_type
  {
  public:
    change_time_type(const T & t) : _t(t) { }
    void operator()(value_type & vt) const { vt._t = _t; }
  private:
    T _t;
  }; // change_time_type


  typedef multi_index_container<
      value_type,
      indexed_by<
        hashed_unique< tag<K>, member < value_type, K , &value_type::_k > >,       // by key
        ordered_non_unique< tag<T>, member < value_type, T , &value_type::_t > >   // by access time
      >
  > t_container;

  typedef typename t_container::template index<K>::type key_index_t;
  typedef typename t_container::template index<T>::type time_index_t;

public:
  typedef typename t_container::size_type size_type;
  typedef typename key_index_t::iterator t_iterator;
  typedef typename time_index_t::iterator t_lru_iterator;

public:
  lru_cache() :
    _size(0),
    _cache_hits(0),
    _cache_misses(0)
  {
  }

  //
  // Key based functions
  //
  inline bool insert(const K & k, const V &v, const T & t)
  {
    key_index_t & key_index = _container.get<K>();
    std::pair<t_iterator, bool> res = key_index.insert(value_type(k, v, t));
    if(res.second) {
        // inserted
        ++_size;
        return true;
    }

    // no luck
    return false;
  }

  inline V find(const K & k, const T & t)
  {
    key_index_t & key_index = _container.get<K>();
    t_iterator it = key_index.find(k);
    if(it != key_index.end()) {
        // hit - update time to mark as recently used
        key_index.modify(it, change_time_type(t));
        ++_cache_hits;
        return (*it)._v;
    }

    // miss
    ++_cache_misses;
    return V();
  }

  inline void use(const K & k, const T & t)
  {
    key_index_t & key_index = _container.get<K>();
    t_iterator it = key_index.find(k);
    if(it != key_index.end()) {
        // update time to mark as recently used
        key_index.modify(it, change_time_type(t));
    }
  }

  inline V erase(const K & k)
  {
    key_index_t & key_index = _container.get<K>();
    t_iterator it = key_index.find(k);
    if(it != key_index.end()) {
        V res = (*it)._v;
        key_index.erase(it);
        --_size;
        return res;
    }
    return V();
  }

  //
  // Time based functions
  //
  inline t_lru_iterator lru_begin()
  {
    time_index_t & time_index = _container.get<T>();
    return time_index.begin();
  }
  inline t_lru_iterator lru_end()
  {
    time_index_t & time_index = _container.get<T>();
    return time_index.end();
  }
  inline t_lru_iterator lru_erase(t_lru_iterator it)
  {
    time_index_t & time_index = _container.get<T>();
    if(it != time_index.end()) {
        it = time_index.erase(it);
        --_size;
    }
    return it;
  }

  //
  // Misc
  //
  inline void clear()
  {
    _container.clear();
    _size = _cache_hits = _cache_misses = 0;
  }

  inline const size_type & get_size() const
  {
    return _size;
  }

  inline const size_type & get_cache_hits() const
  {
    return _cache_hits;
  }

  inline void reset_cache_hits()
  {
    _cache_hits = 0;
  }

  inline const size_type & get_cache_misses() const
  {
    return _cache_misses;
  }

  inline void reset_cache_misses()
  {
    _cache_misses = 0;
  }

private:
  t_container _container;
  size_type   _size;

  // stats
  size_type   _cache_hits;
  size_type   _cache_misses;
}; // class lru_cache


#endif /* COMMON_LRU_CACHE_H_ */
