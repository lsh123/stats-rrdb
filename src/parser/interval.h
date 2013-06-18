/*
 * interval.h
 *
 *  Created on: Jun 6, 2013
 *      Author: aleksey
 */

#ifndef INTERVAL_H_
#define INTERVAL_H_

#include <string>

#include "types.h"

/**
 * Interval string representation:
 *
 * <n> sec|min|hour|day|week|month|year
 *
 */
my::interval_t interval_parse(const std::string & str);
std::string interval_write(const my::interval_t & interval);

std::string interval_get_name(const my::interval_t & unit, const my::interval_t & val);

#endif /* INTERVAL_H_ */
