/*
 * rrdb.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/thread/locks.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include "rrdb.h"
#include "rrdb_metric.h"

#include "parser/statements.h"

#include "config.h"
#include "log.h"
#include "exception.h"

//
//
//
class statement_execute_visitor :
    public boost::static_visitor<std::string>
{
public:
  statement_execute_visitor(const boost::shared_ptr<rrdb> rrdb):
    _rrdb(rrdb)
  {
  }

  std::string operator()(const statement_create & st) const
  {
    _rrdb->create_metric(st._name, st._policy);
    return "OK";
  }

  std::string operator()(const statement_drop & st) const
  {
    _rrdb->drop_metric(st._name);
    return "OK";
  }

  std::string operator()(const statement_show & st) const
  {
    boost::shared_ptr<rrdb_metric> metric = _rrdb->get_metric(st._name);
    return retention_policy_write(metric->get_policy());
  }

  std::string operator()(const statement_show_metrics & st) const
  {
    rrdb::t_metrics_vector metrics = _rrdb->get_metrics(st._like);
    std::ostringstream res;
    BOOST_FOREACH(boost::shared_ptr<rrdb_metric> metric, metrics) {
      res << metric->get_name() << std::endl;
    }
    return res.str();
  }

private:
  const boost::shared_ptr<rrdb> _rrdb;
}; // statement_execute_visitor

//
//
//
rrdb::rrdb(boost::shared_ptr<config> config) :
  _path(config->get<std::string>("rrdb.path", "/var/lib/rrdb")),
  _flush_interval(interval_parse(config->get<std::string>("rrdb.flush_interval", "1 min"))),
  _default_policy(retention_policy_parse(config->get<std::string>("rrdb.default_policy", "1 min FOR 1 day")))
{
  log::write(log::LEVEL_DEBUG, "Starting rrdb");

  log::write(log::LEVEL_INFO, "Started rrdb: flush_interval='%s'", interval_write(_flush_interval).c_str());
  log::write(log::LEVEL_INFO, "Started rrdb: default_policy='%s'", retention_policy_write(_default_policy).c_str());
}

rrdb::~rrdb()
{
}

void rrdb::start()
{
  // ensure folders exist
  boost::filesystem::create_directories(_path);

  statement statement;
  std::string res;

  // TODO:
  statement = statement_parse("CREATE METRIC \"TEST\" KEEP 10 SEC FOR 1 MIN, 1 MIN FOR 1 MONTH;");
  res = boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);
  log::write(log::LEVEL_DEBUG, "RRDB: CREATE metric returned: '%s'", res.c_str());

  statement = statement_parse("SHOW METRICS LIKE \"t\";");
  res = boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);
  log::write(log::LEVEL_DEBUG, "RRDB: SHOW METRICS returned: '%s'", res.c_str());

  statement = statement_parse("SHOW metric \"test\";");
  res = boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);
  log::write(log::LEVEL_DEBUG, "RRDB: SHOW metric returned: '%s'", res.c_str());

  statement = statement_parse("drop metric \"test\";");
  res = boost::apply_visitor(statement_execute_visitor(shared_from_this()), statement);
  log::write(log::LEVEL_DEBUG, "RRDB: DROP metric returned: '%s'", res.c_str());
}

void rrdb::stop()
{
  log::write(log::LEVEL_DEBUG, "Stopping RRDB server");

  log::write(log::LEVEL_INFO, "Stopped RRDB server");
}


boost::shared_ptr<rrdb_metric> rrdb::find_metric(const std::string & name)
{
  // force lower case for names
  std::string name_lc(name);
  boost::algorithm::to_lower(name_lc);

  // lock access to _metrics_map
  boost::shared_ptr<rrdb_metric> res;
  {
    boost::lock_guard<spinlock> guard(_metrics_map_lock);
    t_metrics_map::const_iterator it = _metrics_map.find(name_lc);
    if(it != _metrics_map.end()) {
        res = (*it).second;
    }
  }

  return res;
}

boost::shared_ptr<rrdb_metric> rrdb::get_metric(const std::string & name)
{
  boost::shared_ptr<rrdb_metric> res = this->find_metric(name);
  if(!res) {
      throw exception("The metric '%s' does not exist", name.c_str());
  }
  return res;
}

boost::shared_ptr<rrdb_metric> rrdb::create_metric(const std::string & name, const retention_policy & policy)
{
  // log
  log::write(log::LEVEL_DEBUG, "RRDB: creating metric '%s' with policy '%s'", name.c_str(), retention_policy_write(policy).c_str());

  // force lower case for names
  std::string name_lc(name);
  boost::algorithm::to_lower(name_lc);

  // lock access to _metrics_map
  boost::shared_ptr<rrdb_metric> res(new rrdb_metric(name_lc, policy));
  {
    // make sure there is always only one metric for the name
    boost::lock_guard<spinlock> guard(_metrics_map_lock);
    t_metrics_map::const_iterator it = _metrics_map.find(name_lc);
    if(it != _metrics_map.end()) {
        throw exception("The metric '%s' already exists", name.c_str());
    }
    _metrics_map[name_lc] = res;
  }

  // log
  log::write(log::LEVEL_INFO, "RRDB: created metric '%s' with policy '%s'", name.c_str(), retention_policy_write(policy).c_str());

  return res;
}

void rrdb::drop_metric(const std::string & name)
{
  // log
  log::write(log::LEVEL_DEBUG, "RRDB: dropping metric '%s'", name.c_str());

  // force lower case for names
  std::string name_lc(name);
  boost::algorithm::to_lower(name_lc);

  // lock access to _metrics_map
  boost::shared_ptr<rrdb_metric> res;
  {
    boost::lock_guard<spinlock> guard(_metrics_map_lock);
    t_metrics_map::const_iterator it = _metrics_map.find(name_lc);
    if(it == _metrics_map.end()) {
        throw exception("The metric '%s' does not exist ", name.c_str());
    }
    _metrics_map.erase(it);
  }

  // log
  log::write(log::LEVEL_INFO, "RRDB: dropped metric '%s'", name.c_str());
}

rrdb::t_metrics_vector rrdb::get_metrics(const std::string & like)
{
  // force lower case for names
  std::string like_lc(like);
  boost::algorithm::to_lower(like_lc);

  // lock access to _metrics_map
  rrdb::t_metrics_vector res;
  {
    boost::lock_guard<spinlock> guard(_metrics_map_lock);

    BOOST_FOREACH(t_metrics_map::value_type & v, _metrics_map) {
      if(v.first.find(like_lc) != std::string::npos) {
          res.push_back(v.second);
      }
    }
  }

  // done
  return res;
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
