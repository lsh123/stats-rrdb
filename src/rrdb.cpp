/*
 * rrdb.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#include "rrdb.h"

#include "config.h"
#include "log.h"
#include "parser/statements.h"


rrdb::rrdb(boost::shared_ptr<config> config) :
  _path(config->get<std::string>("rrdb.path", "/var/lib/stats-rrdb"))
{
  log::write(log::LEVEL_INFO, "Starting rrdb");

  std::string flush_interval = config->get<std::string>("rrdb.flush_interval", "1 min");
  _flush_interval = interval_parse(flush_interval.begin(), flush_interval.end());

  std::string default_policy = config->get<std::string>("rrdb.default_policy", "1 min FOR 1 day");
  _default_policy = retention_policy_parse(default_policy.begin(), default_policy.end());

  log::write(log::LEVEL_INFO, "Started rrdb: path='%s'", _path.c_str());
  log::write(log::LEVEL_INFO, "Started rrdb: flush_interval='%s'", interval_write(_flush_interval).c_str());
  log::write(log::LEVEL_INFO, "Started rrdb: default_policy='%s'", retention_policy_write(_default_policy).c_str());

  // TODO:
  std::string s = "drop metric \"test\" ;";
  statement statement;
  statement = statement_parse(s.begin(), s.end());
}

rrdb::~rrdb()
{
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
