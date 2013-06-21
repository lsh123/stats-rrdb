/*
 * spinlock.h
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#ifndef SPINLOCK_H_
#define SPINLOCK_H_

#include <boost/atomic.hpp>
#include <boost/thread/locks.hpp>

class spinlock
{
  typedef enum {
    State_Unlocked = 0,
    State_Locked   = 1
  } lock_state;

public:
  inline spinlock() :
    _state(State_Unlocked)
  {
  }

  inline void lock()
  {
    // busy wait
    while (_state.exchange(State_Locked, boost::memory_order_acquire) == State_Locked) { }
  }

  inline void unlock()
  {
    _state.store(State_Unlocked, boost::memory_order_release);
  }

  inline bool is_locked() const
  {
    return _state == State_Locked;
  }

private:
  boost::atomic<lock_state> _state;
}; // class spinlock


#endif /* SPINLOCK_H_ */
