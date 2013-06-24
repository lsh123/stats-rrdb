/*
 * enable_intrusive_ptr.h
 *
 *  Created on: Jun 23, 2013
 *      Author: aleksey
 */

#ifndef ENABLE_INTRUSIVE_PTR_H_
#define ENABLE_INTRUSIVE_PTR_H_

#include <boost/intrusive_ptr.hpp>
#include <boost/atomic.hpp>
#include <boost/cstdint.hpp>

//
// Very simple ref counter implementation to use with boost::intrusive_ptr<>
//
template<typename Derived>
struct enable_intrusive_ptr
{
  typedef boost::int32_t t_ref_counter;

public:
  //
  // main operations
  //
  inline void add_ref() const
  {
    _ref_count.fetch_add(1, boost::memory_order_relaxed);
  }

  inline void release() const
  {
    if(_ref_count.fetch_sub(1, boost::memory_order_release) == 1) {
      boost::atomic_thread_fence(boost::memory_order_acquire);

      boost::checked_delete(static_cast<Derived const *>(this));
    }
  }

  inline t_ref_counter use_count() const
  {
    return _ref_count.load(boost::memory_order_relaxed);
  }

protected:
  // constructors: overwrite copy constructor: we DO NOT want to copy ref_count
  inline enable_intrusive_ptr() : _ref_count(0) { }
  inline enable_intrusive_ptr(enable_intrusive_ptr<Derived> &) : _ref_count(0) { }

  // asignment operator: again, do NOT copy ref_count
  inline enable_intrusive_ptr &operator =(enable_intrusive_ptr<Derived> const &) { return *this; }

private:
  mutable boost::atomic<t_ref_counter> _ref_count;
}; // enable_intrusive_ptr

//
// The two functions required by intrusive_ptr<>
//
template<typename Derived>
inline void intrusive_ptr_add_ref(enable_intrusive_ptr<Derived> const *that)
{
  that->add_ref();
}

template<typename Derived>
inline void intrusive_ptr_release(enable_intrusive_ptr<Derived> const *that)
{
  that->release();
}


#endif /* ENABLE_INTRUSIVE_PTR_H_ */
