/*
 * rrdb_metric_tuple.cpp
 *
 *  Created on: Jun 13, 2013
 *      Author: aleksey
 */
#include <ostream>
#include <cmath>
#include <boost/foreach.hpp>

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


void rrdb_metric_tuple_write_header(
    const t_tuple_expr_list & expr_list,
    t_memory_buffer & res
) {
  // ts is always first
  res << "ts,";

  // field names
  BOOST_FOREACH(const t_tuple_field_extractor & func, expr_list) {
    func(NULL, res);
  }

  // end of record/line
  res << std::endl;
}

void rrdb_metric_tuple_write_tuple(
    const t_tuple_expr_list & expr_list,
    const t_rrdb_metric_tuple & tuple,
    t_memory_buffer & res
) {
  // ts is always first
  res << tuple._ts << ",";

  // fields
  BOOST_FOREACH(const t_tuple_field_extractor & func, expr_list) {
    func(&tuple, res);
  }

  // end of record/line
  res << std::endl;
}

void rrdb_metric_tuple_write_all(t_rrdb_metric_tuple const * tuple, t_memory_buffer & res)
{
  // write column name(s) only?
  if(!tuple) {
      res << "count,sum,avg,stddev,min,max";
      return;
  }

  // calculate
  my::value_t avg, stddev;
  if(tuple->_count > 0) {
      avg = tuple->_sum / tuple->_count;
      stddev = sqrt(tuple->_sum_sqr / tuple->_count - avg * avg) ;
  } else {
      avg = stddev = 0;
  }

  // write
  res  << tuple->_count
       << ','
       << tuple->_sum
       << ','
       << avg
       << ','
       << stddev
       << ','
       << tuple->_min
       << ','
       << tuple->_max
  ;
}

void rrdb_metric_tuple_write_min(t_rrdb_metric_tuple const * tuple, t_memory_buffer & res)
{
  // write column name(s) only?
  if(!tuple) {
      res << "min";
      return;
  }

  // write
  res  << tuple->_min;
}

void rrdb_metric_tuple_write_max(t_rrdb_metric_tuple const * tuple, t_memory_buffer & res)
{
  // write column name(s) only?
  if(!tuple) {
      res << "max";
      return;
  }

  // write
  res  << tuple->_max;
}

void rrdb_metric_tuple_write_avg(t_rrdb_metric_tuple const * tuple, t_memory_buffer & res)
{
  // write column name(s) only?
  if(!tuple) {
      res << "avg";
      return;
  }

  // calculate
  my::value_t avg;
  if(tuple->_count > 0) {
      avg = tuple->_sum / tuple->_count;
  } else {
      avg = 0;
  }

  // write
  res  << avg;
}

void rrdb_metric_tuple_write_sum(t_rrdb_metric_tuple const * tuple, t_memory_buffer & res)
{
  // write column name(s) only?
  if(!tuple) {
      res << "sum";
      return;
  }

  // write
  res  << tuple->_sum;
}

void rrdb_metric_tuple_write_count(t_rrdb_metric_tuple const * tuple, t_memory_buffer & res)
{
  // write column name(s) only?
  if(!tuple) {
      res << "count";
      return;
  }

  // write
  res  << tuple->_count;
}

void rrdb_metric_tuple_write_stddev(t_rrdb_metric_tuple const * tuple, t_memory_buffer & res)
{
  // write column name(s) only?
  if(!tuple) {
      res << "stddev";
      return;
  }

  // calculate
  my::value_t stddev;
  if(tuple->_count > 0) {
      my::value_t avg = tuple->_sum / tuple->_count;
      stddev = sqrt(tuple->_sum_sqr / tuple->_count - avg * avg) ;
  } else {
      stddev = 0;
  }

  // write
  res  << stddev;
}
