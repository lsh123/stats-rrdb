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
    log::write(log::LEVEL_DEBUG, "Execure 'create': '%s' => '%s'", st._name.c_str(), retention_policy_write(st._policy).c_str());
  }
  void operator()(const statement_drop & st) const
  {
    log::write(log::LEVEL_DEBUG, "Execure 'drop': '%s'", st._name.c_str());
  }
  void operator()(const statement_show & st) const
  {
    log::write(log::LEVEL_DEBUG, "Execure 'show': '%s'", st._name.c_str());
  }

private:
    const boost::shared_ptr<rrdb> _rrdb;
}; // statement_execute_visitor

//
//
//
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
}

rrdb::~rrdb()
{
}

void rrdb::start()
{
  statement statement;
  std::string s;

  // TODO:
  s = "create metric \"test\" KEEP 10 sec FOR 1 min, 1 min for 1 month;";
  statement = statement_parse(s.begin(), s.end());
  boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);

  s = "SHOW metric \"test\";";
  statement = statement_parse(s.begin(), s.end());
  boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);


  s = "drop metric \"test\";";
  statement = statement_parse(s.begin(), s.end());
  boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);
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
