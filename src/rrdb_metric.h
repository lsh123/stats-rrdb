/*
 * rrdb_metric.h
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_H_
#define RRDB_METRIC_H_

#include <string>
#include <boost/shared_ptr.hpp>

#include "spinlock.h"
#include "parser/retention_policy.h"

// forward
class rrdb;

//
// RRDB Metric file header format:
//
// 0x99DB   - magic byte
// 0x0000   - status byte (unused)
// 0xNNNN   - name length
// 0xNNNN   - policy size
// <name>   - aligned to 4 bytes
// (<64 bit freq> <64 bit count>)* - policy
//
class rrdb_metric
{
public:
  rrdb_metric();
  rrdb_metric(const std::string & name, const retention_policy & policy);
  virtual ~rrdb_metric();

  std::string get_name();
  retention_policy get_policy();

private:
  spinlock          _lock;
  std::string       _name;
  retention_policy  _policy;
}; // class rrdb_metric

#endif /* RRDB_METRIC_H_ */
