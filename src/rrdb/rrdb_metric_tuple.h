/*
 * rrdb_metric_tuple.h
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_TUPLE_H_
#define RRDB_METRIC_TUPLE_H_

#include <boost/cstdint.hpp>
#include <boost/shared_array.hpp>

#include "common/types.h"
#include "common/memory_buffer.h"

//
// Value
//
typedef struct t_rrdb_metric_tuple_ {
  my::time_t    _ts;                // block timestamp
  my::value_t   _count;             // number of data points aggregated
  my::value_t   _sum;               // sum(data point value)
  my::value_t   _sum_sqr;           // sum(sqr(data point value))
  my::value_t   _min;               // min(data point value)
  my::value_t   _max;               // max(data point value)
} t_rrdb_metric_tuple;

typedef boost::shared_array< t_rrdb_metric_tuple > rrdb_metric_tuples_t;

void rrdb_metric_tuple_update(t_rrdb_metric_tuple & tuple, const my::value_t & value);
void rrdb_metric_tuple_update(t_rrdb_metric_tuple & tuple, const t_rrdb_metric_tuple & other);
void rrdb_metric_tuple_normalize(t_rrdb_metric_tuple & tuple, const my::value_t & factor);

void rrdb_metric_tuple_write_header(t_memory_buffer & res);
void rrdb_metric_tuple_write(const t_rrdb_metric_tuple & tuple, t_memory_buffer & res);

#endif /* RRDB_METRIC_TUPLE_H_ */
