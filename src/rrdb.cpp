/*
 * rrdb.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#include "rrdb.h"

#include "config.h"
#include "log.h"

rrdb::rrdb(boost::shared_ptr<config> config)
{
  log::write(log::LEVEL_INFO, "Starting rrdb");

  log::write(log::LEVEL_INFO, "Started rrdb");
}

rrdb::~rrdb()
{
  // TODO Auto-generated destructor stub
}

rrdb::t_result_buffers rrdb::execute_long_command(const std::vector<char> & buffer)
{
  rrdb::t_result_buffers res;
  res.push_back(boost::asio::buffer("OK"));
  return res;
}

void rrdb::execute_short_command(const std::vector<char> & buffer)
{

}
