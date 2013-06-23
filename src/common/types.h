/*
 * types.h
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

namespace my {

//
// Timestamps
//
typedef boost::int64_t  time_t;

// checks overlaps for two intervals and returns:
//      -1 if [ts1_b, ts1_e) < [ts2_b, ts2_e) (i.e. ts1 is earlier than ts2)
//       1 if [ts2_b, ts2_e) < [ts1_b, ts1_e) (i.e. ts1 is later than ts2)
//       0 if the two overlap
inline int interval_overlap(const time_t & ts1_b, const time_t & ts1_e, const time_t & ts2_b, const time_t & ts2_e)
{
  if(ts1_e <= ts2_b) {
      return -1;
  } else if(ts2_e <= ts1_b) {
      return 1;
  }
  return 0;
}


//
// Interval: number of seconds between two timestamps
//
typedef boost::uint32_t interval_t;

#define INTERVAL_SEC    1
#define INTERVAL_MIN    (60 * INTERVAL_SEC)
#define INTERVAL_HOUR   (60 * INTERVAL_MIN)
#define INTERVAL_DAY    (24 * INTERVAL_HOUR)
#define INTERVAL_WEEK   (7 * INTERVAL_DAY)
#define INTERVAL_MONTH  (30 * INTERVAL_DAY)
#define INTERVAL_YEAR   (365 * INTERVAL_DAY)


//
// Size
//
typedef boost::uint64_t size_t;


//
// Metric value
//
typedef double value_t;


//
// Bitmask Ops
//
template<typename T>
inline void bitmask_set(T & v, const T & mask) { v |= mask; }

template<typename T>
inline void bitmask_clear(T & v, const T & mask) { v &= ~mask; }

template<typename T>
inline bool bitmask_check(const T & v, const T & mask) { return (v & mask); }

template<typename T>
inline bool bitmask_check_all(const T & v, const T & mask) { return (v & mask) == mask; }

//
// Filename - shared_ptr to save space
//
typedef boost::shared_ptr<std::string> filename_t;

} // namespace my

#endif /* COMMON_TYPES_H_ */
