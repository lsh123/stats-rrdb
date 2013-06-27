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
typedef struct t_retention_policy_elem_
{
  my::interval_t _freq;
  my::interval_t _duration;


  inline t_retention_policy_elem_(
      const my::interval_t & freq = 0,
      const my::interval_t & duration = 0
  ) :
    _freq(freq),
    _duration(duration)
  {
  }
} t_retention_policy_elem;
typedef std::vector<t_retention_policy_elem> t_retention_policy;

t_retention_policy retention_policy_parse(const std::string & str);
std::string retention_policy_write(const t_retention_policy & r_p);
void retention_policy_validate(const t_retention_policy & r_p);

#endif /* RETENTION_POLICY_H_ */
