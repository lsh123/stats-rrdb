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

//
//
//
class statement_execute_visitor :
    public boost::static_visitor<>
{
public:
  statement_execute_visitor(const boost::shared_ptr<rrdb> rrdb):
    _rrdb(rrdb)
  {
  }

  void operator()(const statement_create & st) const
  {
    _rrdb->create_metric(st._name, st._policy);
  }
  void operator()(const statement_drop & st) const
  {
    _rrdb->drop_metric(st._name);
  }
  void operator()(const statement_show & st) const
  {
    _rrdb->get_metric_policy(st._name);
  }

private:
    const boost::shared_ptr<rrdb> _rrdb;
}; // statement_execute_visitor

//
//
//
rrdb::rrdb(boost::shared_ptr<config> config) :
  _path(config->get<std::string>("rrdb.path", "/var/lib/stats-rrdb")),
  _flush_interval(interval_parse(config->get<std::string>("rrdb.flush_interval", "1 min"))),
  _default_policy(retention_policy_parse(config->get<std::string>("rrdb.default_policy", "1 min FOR 1 day")))
{
  log::write(log::LEVEL_INFO, "Starting rrdb");

  log::write(log::LEVEL_INFO, "Started rrdb: path='%s'", _path.c_str());
  log::write(log::LEVEL_INFO, "Started rrdb: flush_interval='%s'", interval_write(_flush_interval).c_str());
  log::write(log::LEVEL_INFO, "Started rrdb: default_policy='%s'", retention_policy_write(_default_policy).c_str());
}

rrdb::~rrdb()
{
}

void rrdb::start()
{
  statement statement;

  // TODO:
  statement = statement_parse("CREATE METRIC \"TEST\" KEEP 10 SEC FOR 1 MIN, 1 MIN FOR 1 MONTH;");
  boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);

  statement = statement_parse("SHOW metric \"test\";");
  boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);

  statement = statement_parse("drop metric \"test\";");
  boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);
}

void rrdb::create_metric(const std::string & name, const retention_policy & policy)
{
  log::write(log::LEVEL_INFO, "RRDB: create metric '%s' with policy '%s'", name.c_str(), retention_policy_write(policy).c_str());
}

void rrdb::drop_metric(const std::string & name)
{
  log::write(log::LEVEL_INFO, "RRDB: drop metric '%s'", name.c_str());
}

retention_policy rrdb::get_metric_policy(const std::string & name)
{
  log::write(log::LEVEL_DEBUG, "RRDB: get metric '%s'", name.c_str());
  return retention_policy();
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
