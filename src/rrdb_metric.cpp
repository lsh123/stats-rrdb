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

bool rrdb_metric::is_dirty()
{
  // TODO:
  boost::lock_guard<spinlock> guard(_lock);
  return false;
}

void rrdb_metric::set_dirty()
{
  // TODO:
  boost::lock_guard<spinlock> guard(_lock);
}

bool rrdb_metric::is_deleted()
{
  // TODO:
  boost::lock_guard<spinlock> guard(_lock);
  return false;
}

void rrdb_metric::set_deleted()
{
  // TODO:
  boost::lock_guard<spinlock> guard(_lock);
}

void rrdb_metric::save_file()
{
  // check if deleted meantime
  if(this->is_deleted()) {
      return;
  }

  // start
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' saving file", this->get_name().c_str());

  // done
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' saved file", this->get_name().c_str());
}

void rrdb_metric::delete_file()
{
  // mark as deleted in case the flush thread picks it up in the meantime
  this->set_deleted();

  // start
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' deleting file", this->get_name().c_str());

  // done
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' deleted file", this->get_name().c_str());
}
