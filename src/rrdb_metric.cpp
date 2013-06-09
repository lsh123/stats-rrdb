/*
 * rrdb_metric.cpp
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#include <boost/thread/locks.hpp>

#include "rrdb_metric.h"
#include "rrdb.h"


#include "log.h"
#include "exception.h"


rrdb_metric::rrdb_metric()
{
}

rrdb_metric::rrdb_metric(const std::string & name, const retention_policy & policy) :
    _name(name),
    _policy(policy)
{
}

rrdb_metric::~rrdb_metric()
{
  // TODO Auto-generated destructor stub
}

// use copy here to avoid problems with MT
std::string rrdb_metric::get_name()
{
  log::write(log::LEVEL_DEBUG, "DEBUG: get_name");

  boost::lock_guard<spinlock> guard(_lock);
  return _name;
}

// use copy here to avoid problems with MT
retention_policy rrdb_metric::get_policy()
{
  log::write(log::LEVEL_DEBUG, "DEBUG: get_policy");

  boost::lock_guard<spinlock> guard(_lock);
  return _policy;
}


