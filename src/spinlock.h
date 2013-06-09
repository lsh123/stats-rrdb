/*
 * spinlock.h
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#ifndef SPINLOCK_H_
#define SPINLOCK_H_


#include <boost/smart_ptr/detail/spinlock.hpp>

// boost/xxx/detail/* are internal boost file, shouldn't really be used outside
// of boost itself. But I am lazy to upgrade to 1.53
class spinlock
{
  typedef boost::detail::spinlock t_base;

public:
  inline spinlock()
  {
    // start unlocked
    _l.v_ = 0;
  }

  inline bool try_lock()
  {
    return _l.try_lock();
  }

  inline void lock()
  {
    _l.lock();
  }

  inline void unlock()
  {
    _l.unlock();
  }

private:
  boost::detail::spinlock _l;
}; // class spinlock


#endif /* SPINLOCK_H_ */
