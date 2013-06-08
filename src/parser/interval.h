/*
 * interval.h
 *
 *  Created on: Jun 6, 2013
 *      Author: aleksey
 */

#ifndef INTERVAL_H_
#define INTERVAL_H_

#include <string>
#include <boost/cstdint.hpp>

#define INTERVAL_SEC    1
#define INTERVAL_MIN    (60 * INTERVAL_SEC)
#define INTERVAL_HOUR   (60 * INTERVAL_MIN)
#define INTERVAL_DAY    (24 * INTERVAL_HOUR)
#define INTERVAL_WEEK   (7 * INTERVAL_DAY)
#define INTERVAL_MONTH  (30 * INTERVAL_DAY)
#define INTERVAL_YEAR   (365 * INTERVAL_DAY)

/**
 * Interval string representation:
 *
 * <n> sec|min|hour|day|week|month|year
 *
 */
typedef boost::uint64_t interval_t;

interval_t interval_parse(std::string::const_iterator beg, std::string::const_iterator end);
std::string interval_write(const interval_t & interval);

std::string interval_get_name(const interval_t & unit, const interval_t & val);

#endif /* INTERVAL_H_ */
