/*
 * rrdb_metric_tuple.h
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_TUPLE_H_
#define RRDB_METRIC_TUPLE_H_

#include <boost/cstdint.hpp>

#include "common/types.h"
#include "common/exception.h"
#include "common/memory_buffer.h"
#include "common/enable_intrusive_ptr.h"

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

typedef struct t_rrdb_metric_tuples_ :
    public enable_intrusive_ptr<t_rrdb_metric_tuples_>
{
  typedef my::size_t size_type;

public:
  // constuctor/destructor
  inline t_rrdb_metric_tuples_(const size_type & size = 0) :
    _tuples(NULL),
    _size(0)
  {
    if(size > 0) {
        this->reset(new t_rrdb_metric_tuple[size](), size);
        this->zero();
    }
  }
  inline ~t_rrdb_metric_tuples_()
  {
    this->reset();
  }

  inline void zero()
  {
    // shouldn't be doing it in C++ but it's the fastest way
    if(_tuples) {
        memset(this->get(), 0, this->get_memory_size());
    }
  }

  // access
  inline t_rrdb_metric_tuple & operator[] (const size_type & pos)
  {
    CHECK_AND_THROW(pos < _size);
    return _tuples[pos];
  }
  inline const t_rrdb_metric_tuple& operator[] (const size_type & pos) const
  {
    CHECK_AND_THROW(pos < _size);
    return _tuples[pos];
  }

  inline size_type get_size() const { return _size; }
  inline size_type get_memory_size() const { return get_size() * sizeof(t_rrdb_metric_tuple); }

  inline t_rrdb_metric_tuple * get() { return _tuples; }
  inline t_rrdb_metric_tuple const * get() const { return _tuples; }

  inline t_rrdb_metric_tuple * get_at(const size_type & pos)
  {
    CHECK_AND_THROW(pos < _size);
    return &(_tuples[pos]);
  }
  inline t_rrdb_metric_tuple const * get_at(const size_type & pos) const
  {
    CHECK_AND_THROW(pos < _size);
    return &(_tuples[pos]);
  }

private:
  // disable copy constructor and assignment operator
  t_rrdb_metric_tuples_(t_rrdb_metric_tuples_ const &);
  t_rrdb_metric_tuples_ &operator =(t_rrdb_metric_tuples_ const &);

  // assign tuples
  inline void reset(t_rrdb_metric_tuple * tuples = NULL, const size_type & size = 0)
  {
    if(_tuples) {
        delete[] _tuples;
    }
    _tuples = tuples;
    _size = size;
  }

private:
  t_rrdb_metric_tuple * _tuples;
  size_type             _size;
} t_rrdb_metric_tuples;

// and the actual thing we are going to use
typedef boost::intrusive_ptr< t_rrdb_metric_tuples > t_rrdb_metric_tuples_ptr;

void rrdb_metric_tuple_update(t_rrdb_metric_tuple & tuple, const my::value_t & value);
void rrdb_metric_tuple_update(t_rrdb_metric_tuple & tuple, const t_rrdb_metric_tuple & other);
void rrdb_metric_tuple_update(t_rrdb_metric_tuple & tuple, const t_rrdb_metric_tuple & other, const my::value_t & factor);

void rrdb_metric_tuple_write_header(t_memory_buffer & res);
void rrdb_metric_tuple_write(const t_rrdb_metric_tuple & tuple, t_memory_buffer & res);

#endif /* RRDB_METRIC_TUPLE_H_ */
