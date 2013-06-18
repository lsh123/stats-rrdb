/*
 * retention_policy.h
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#ifndef RETENTION_POLICY_H_
#define RETENTION_POLICY_H_

#include <vector>
#include "interval.h"

/**
 * Retention policy string representation:
 *
 * <interval freq> FOR <interval duration> (, <interval freq> FOR <interval duration>)*
 *
 */
typedef struct retention_policy_elem_
{
  my::interval_t _freq;
  my::interval_t _duration;
} retention_policy_elem;
typedef std::vector<retention_policy_elem> retention_policy;

retention_policy retention_policy_parse(const std::string & str);
std::string retention_policy_write(const retention_policy & r_p);
void retention_policy_validate(const retention_policy & r_p);

#endif /* RETENTION_POLICY_H_ */
