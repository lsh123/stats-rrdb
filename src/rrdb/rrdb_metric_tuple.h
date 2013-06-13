/*
 * rrdb_metric_tuple.h
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_TUPLE_H_
#define RRDB_METRIC_TUPLE_H_

#include <boost/cstdint.hpp>

//
// Value
//
typedef struct rrdb_metric_tuple_t_ {
  boost::int64_t    _ts;                // block timestamp
  boost::int64_t    _count;             // number of data points aggregated
  double            _sum;               // sum(data point value)
  double            _sum_sqr;           // sum(sqr(data point value))
  double            _min;               // min(data point value)
  double            _max;               // max(data point value)
} rrdb_metric_tuple_t;



#endif /* RRDB_METRIC_TUPLE_H_ */
