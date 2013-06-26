/*
 * spinlock.h
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#ifndef COMMON_SPINLOCK_H_
#define COMMON_SPINLOCK_H_

#include <boost/atomic.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread.hpp>

#define BUSY_SPIN_COUNT 3

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
    // semi-busy wait
    register int ii = 0;
    while(_state.exchange(State_Locked, boost::memory_order_acquire) == State_Locked) {
        if((++ii) > BUSY_SPIN_COUNT) {
            boost::this_thread::yield();
            ii = 0;
        }
    }
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


#endif /* COMMON_SPINLOCK_H_ */
