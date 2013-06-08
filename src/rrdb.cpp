/*
 * rrdb.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#include "rrdb.h"

#include "config.h"
#include "log.h"

#include "parser/interval.h"

rrdb::rrdb(boost::shared_ptr<config> config) :
  _path(config->get<std::string>("rrdb.path", "/var/lib/stats-rrdb"))
{
  log::write(log::LEVEL_INFO, "Starting rrdb");

  std::string flush_interval = config->get<std::string>("rrdb.flush_interval", "1 min");
  _flush_interval = interval_parse(flush_interval.begin(), flush_interval.end());

  log::write(log::LEVEL_INFO, "Started rrdb: path='%s', flush_interval='%s'", _path.c_str(), interval_write(_flush_interval).c_str());
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
