/*
 * rrdb_metric_tuple.cpp
 *
 *  Created on: Jun 13, 2013
 *      Author: aleksey
 */

#include "rrdb_metric_tuple.h"

void rrdb_metric_tuple_update(rrdb_metric_tuple_t & tuple, const double & value)
{
  if(tuple._min > value || tuple._count == 0) {
      tuple._min = value;
  }
  if(tuple._max < value || tuple._count == 0) {
      tuple._max = value;
  }
  tuple._sum += value;
  tuple._sum_sqr += value * value;
  ++tuple._count;
}

void rrdb_metric_tuple_update(rrdb_metric_tuple_t & tuple, const rrdb_metric_tuple_t & other)
{
  if(tuple._min > other._min || tuple._count == 0) {
      tuple._min = other._min;
  }
  if(tuple._max < other._max || tuple._count == 0) {
      tuple._max = other._max;
  }
  tuple._sum     += other._sum;
  tuple._sum_sqr += other._sum_sqr;
  tuple._count   += other._count;
}



