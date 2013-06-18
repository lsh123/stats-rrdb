/*
 * rrdb_metric_tuple.h
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_TUPLE_H_
#define RRDB_METRIC_TUPLE_H_

#include <boost/cstdint.hpp>

#include "types.h"

//
// Value
//
typedef struct rrdb_metric_tuple_t_ {
  my::time_t    _ts;                // block timestamp
  my::value_t   _count;             // number of data points aggregated
  my::value_t   _sum;               // sum(data point value)
  my::value_t   _sum_sqr;           // sum(sqr(data point value))
  my::value_t   _min;               // min(data point value)
  my::value_t   _max;               // max(data point value)
} rrdb_metric_tuple_t;

void rrdb_metric_tuple_update(rrdb_metric_tuple_t & tuple, const my::value_t & value);
void rrdb_metric_tuple_update(rrdb_metric_tuple_t & tuple, const rrdb_metric_tuple_t & other);
void rrdb_metric_tuple_normalize(rrdb_metric_tuple_t & tuple, const my::value_t & factor);


#endif /* RRDB_METRIC_TUPLE_H_ */
