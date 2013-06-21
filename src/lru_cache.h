/*
 * lru_cache.h
 *
 *  Created on: Jun 20, 2013
 *      Author: aleksey
 */

#ifndef LRU_CACHE_H_
#define LRU_CACHE_H_

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

#include "types.h"

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
    void operator()(value_type & vt) { vt._t = _t; }
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
  //
  // Key based functions
  //
  void insert(const K & k, const V &v, const T & t)
  {
    key_index_t & index_by_key = _container.get<K>();
    index_by_key.insert(value_type(k, v, t));
  }

  t_iterator find(const K & k) const
  {
    const key_index_t & index_by_key = _container.get<K>();
    return index_by_key.find(k);
  }

  V use(t_iterator & it, const T & t)
  {
    key_index_t & index_by_key = _container.get<K>();
    index_by_key.modify(it, change_time_type(t));
    return (*it)._v;
  }

  t_iterator end() const
  {
    const key_index_t & index_by_key = _container.get<K>();
    return index_by_key.end();
  }

  //
  // Time based functions
  //
  t_lru_iterator lru_begin()
  {
    time_index_t & time_index_t = _container.get<T>();
    return time_index_t.begin();
  }
  t_lru_iterator lru_end()
  {
    time_index_t & time_index_t = _container.get<T>();
    return time_index_t.end();
  }
  t_lru_iterator lru_erase(t_lru_iterator it)
  {
    time_index_t & time_index_t = _container.get<T>();
    return time_index_t.erase(it);
  }

  //
  // Misc
  //
  void clear()
  {
    _container.clear();
  }

  size_type size() const
  {
    return _container.size();
  }


private:
  t_container _container;
}; // class lru_cache


#endif /* LRU_CACHE_H_ */
