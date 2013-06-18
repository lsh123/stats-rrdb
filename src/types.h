/*
 * types.h
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <boost/cstdint.hpp>

namespace my {

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

} // namespace my

#endif /* TYPES_H_ */
