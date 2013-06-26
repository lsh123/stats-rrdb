/*
 * rrdb_metric_tuple.cpp
 *
 *  Created on: Jun 13, 2013
 *      Author: aleksey
 */
#include <ostream>
#include <cmath>

#include "rrdb/rrdb_metric_tuple.h"

#include "common/log.h"

void rrdb_metric_tuple_update(t_rrdb_metric_tuple & tuple, const my::value_t & value)
{
  if(tuple._min > value || tuple._count == 0) {
      tuple._min = value;
  }
  if(tuple._max < value || tuple._count == 0) {
      tuple._max = value;
  }
  tuple._sum     += value;
  tuple._sum_sqr += value * value;
  ++tuple._count;
}

void rrdb_metric_tuple_update(t_rrdb_metric_tuple & tuple, const t_rrdb_metric_tuple & other)
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

void rrdb_metric_tuple_update(t_rrdb_metric_tuple & tuple, const t_rrdb_metric_tuple & other, const my::value_t & factor)
{
  // leave alone min/max
  if(tuple._min > other._min || tuple._count == 0) {
      tuple._min = other._min;
  }
  if(tuple._max < other._max || tuple._count == 0) {
      tuple._max = other._max;
  }

  // but account for factor for sum/count/..
  tuple._sum     += factor * other._sum;
  tuple._sum_sqr += factor * other._sum_sqr;
  tuple._count   += factor * other._count;
}


void rrdb_metric_tuple_write_header(t_memory_buffer & res)
{
  res << "ts,count,sum,avg,stddev,min,max" << std::endl;
}

void rrdb_metric_tuple_write(const t_rrdb_metric_tuple & tuple, t_memory_buffer & res)
{
  // calculate
  my::value_t avg, stddev;
  if(tuple._count > 0) {
      avg = tuple._sum / tuple._count;
      stddev = sqrt(tuple._sum_sqr / tuple._count - avg * avg) ;
  } else {
      avg = stddev = 0;
  }

  // write
  res  << tuple._ts
       << ','
       << tuple._count
       << ','
       << tuple._sum
       << ','
       << avg
       << ','
       << stddev
       << ','
       << tuple._min
       << ','
       << tuple._max
       << std::endl;
  ;
}
