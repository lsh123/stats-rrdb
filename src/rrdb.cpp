/*
 * rrdb.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#include <sstream>

#include <boost/thread/locks.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

#include "rrdb.h"
#include "rrdb_metric.h"

#include "parser/statements.h"

#include "config.h"
#include "log.h"
#include "exception.h"

//
//
//
class statement_execute_visitor : public boost::static_visitor<std::string>
{
public:
  statement_execute_visitor(const boost::shared_ptr<rrdb> rrdb) :
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
    std::vector<std::string> metrics = _rrdb->get_metrics(st._like);
    std::ostringstream res;
    BOOST_FOREACH(const std::string & name, metrics){
      res << name << ";";
    }
    return res.str();
  }

private:
  const boost::shared_ptr<rrdb> _rrdb;
};
// statement_execute_visitor

//
//
//
rrdb::rrdb(boost::shared_ptr<config> config) :
    _path(config->get<std::string>("rrdb.path", "/var/lib/rrdb")), _flush_interval(
        interval_parse(
            config->get<std::string>("rrdb.flush_interval", "1 min"))), _default_policy(
        retention_policy_parse(
            config->get<std::string>("rrdb.default_policy", "1 min FOR 1 day")))
{
  log::write(log::LEVEL_DEBUG, "Starting rrdb");

  log::write(log::LEVEL_INFO, "Started rrdb: flush_interval='%s'", interval_write(_flush_interval).c_str());
  log::write(log::LEVEL_INFO, "Started rrdb: default_policy='%s'", retention_policy_write(_default_policy).c_str());
}

rrdb::~rrdb()
{
  this->stop();
}

void rrdb::test()
{
  statement statement;
  std::string res;

  // TODO:
  statement = statement_parse(
      "CREATE METRIC \"TEST\" KEEP 10 SEC FOR 1 MIN, 1 MIN FOR 1 MONTH;");
  res = boost::apply_visitor(statement_execute_visitor(shared_from_this()),
      statement);
  log::write(log::LEVEL_DEBUG, "RRDB: CREATE metric returned: '%s'",
      res.c_str());

  statement = statement_parse("SHOW METRICS LIKE \"t\";");
  res = boost::apply_visitor(statement_execute_visitor(shared_from_this()),
      statement);
  log::write(log::LEVEL_DEBUG, "RRDB: SHOW METRICS returned: '%s'",
      res.c_str());

  statement = statement_parse("SHOW metric \"test\";");
  res = boost::apply_visitor(statement_execute_visitor(shared_from_this()),
      statement);
  log::write(log::LEVEL_DEBUG, "RRDB: SHOW metric returned: '%s'", res.c_str());

  statement = statement_parse("drop metric \"test\";");
  res = boost::apply_visitor(statement_execute_visitor(shared_from_this()),
      statement);
  log::write(log::LEVEL_DEBUG, "RRDB: DROP metric returned: '%s'", res.c_str());
}

bool rrdb::is_running()
{
  return _flush_to_disk_thread;
}

void rrdb::start()
{
  if(this->is_running()) {
      return;
  }

  log::write(log::LEVEL_DEBUG, "Starting RRDB server");

  _flush_to_disk_thread.reset(new boost::thread(boost::bind(&rrdb::flush_to_disk_thread, this)));

  log::write(log::LEVEL_INFO, "Started RRDB server");

  this->test();
}

void rrdb::stop()
{
  if(!this->is_running()) {
      return;
  }

  log::write(log::LEVEL_DEBUG, "Stopping RRDB server");

  // stop flush thread
  _flush_to_disk_thread->interrupt();
  _flush_to_disk_thread->join();
  _flush_to_disk_thread.reset();

  // flush one more time
  this->flush_to_disk();

  log::write(log::LEVEL_INFO, "Stopped RRDB server");
}

void rrdb::flush_to_disk_thread()
{
  // log
  log::write(log::LEVEL_INFO, "RRDB Flush thread started");

  // try/catch to get any error reported
  try {
      while (!boost::this_thread::interruption_requested()) {
          {
            boost::this_thread::disable_interruption d;
            this->flush_to_disk();
          }

          boost::this_thread::sleep(boost::posix_time::seconds(this->_flush_interval));
      }
  } catch (boost::thread_interrupted & e) {
      log::write(log::LEVEL_DEBUG, "RRDB Flush thread was interrupted");
  } catch (std::exception & e) {
      log::write(log::LEVEL_ERROR, "RRDB Flush thread exception: %s", e.what());
      throw e;
  } catch (...) {
      log::write(log::LEVEL_ERROR, "RRDB Flush thread un-handled exception");
      throw;
  }

  // done
  log::write(log::LEVEL_INFO, "RRDB Flush thread stopped");
}

void rrdb::flush_to_disk()
{
  log::write(log::LEVEL_DEBUG, "Flushing to disk");

  t_metrics_vector dirty_metrics = this->get_dirty_metrics();
  BOOST_FOREACH(boost::shared_ptr<rrdb_metric> metric, dirty_metrics) {
    try {
        metric->save_file(_path);
    } catch(std::exception & e) {
      log::write(log::LEVEL_ERROR, "Exception saving metric '%s': %s", metric->get_name().c_str(), e.what());
    }
  }

  log::write(log::LEVEL_DEBUG, "Flushed to disk");
}

boost::shared_ptr<rrdb_metric> rrdb::find_metric(const std::string & name)
{
  // force lower case for names
  std::string name_lc(name);
  boost::algorithm::to_lower(name_lc);

  return this->find_metric_lc(name_lc);
}

boost::shared_ptr<rrdb_metric> rrdb::find_metric_lc(const std::string & name_lc)
{
  // search in the map first: lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    t_metrics_map::const_iterator it = _metrics.find(name_lc);
    if(it != _metrics.end()) {
        return (*it).second;
    }
  }

  // search on disk
  boost::shared_ptr<rrdb_metric> res;
  try {
      log::write(log::LEVEL_DEBUG, "Trying to load metric '%s' from disk", name_lc.c_str());
      res = rrdb_metric::load_file(_path, name_lc);
  } catch(exception & e) {
      // ignore any exceptions
      log::write(log::LEVEL_ERROR, "Exception while loading metric '%s': %s", name_lc.c_str(), e.what());
      return boost::shared_ptr<rrdb_metric>();
  } catch(std::exception & e) {
      // ignore any exceptions
      log::write(log::LEVEL_DEBUG, "Exception while loading metric '%s': %s", name_lc.c_str(), e.what());
      return boost::shared_ptr<rrdb_metric>();
  }

  // try to insert into the map, lock again
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    t_metrics_map::const_iterator it = _metrics.find(name_lc);
    if(it != _metrics.end()) {
        // someone already inserted it, drop the one we just loaded
        // and use one in the metrics map
        return (*it).second;
    }

    // insert loaded metric into map and return
    _metrics[name_lc] = res;
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

  // try to find the metric
  boost::shared_ptr<rrdb_metric> res = this->find_metric_lc(name_lc);
  if(res) {
      throw exception("The metric '%s' already exists", name.c_str());
  }

  // create new and try to insert into map, lock access to _metrics
  res.reset(new rrdb_metric(name_lc, policy));
  {
    // make sure there is always only one metric for the name
    boost::lock_guard<spinlock> guard(_metrics_lock);
    t_metrics_map::const_iterator it = _metrics.find(name_lc);
    if(it != _metrics.end()) {
        // someone inserted it in the meantime
        throw exception("The metric '%s' already exists", name.c_str());
    }
    _metrics[name_lc] = res;
  }

  // create file - outside the spin lock
  res->save_file(_path);

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

  // try to find the metric
  boost::shared_ptr<rrdb_metric> res = this->find_metric_lc(name_lc);
  if(!res) {
      throw exception("The metric '%s' does not exists", name.c_str());
  }

  // delete file - outside the spin lock
  res->delete_file(_path);

  // lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    _metrics.erase(name_lc);
  }

  // log
  log::write(log::LEVEL_INFO, "RRDB: dropped metric '%s'", name.c_str());
}

std::vector<std::string> rrdb::get_metrics(const std::string & like)
{
  // force lower case for names
  std::string like_lc(like);
  boost::algorithm::to_lower(like_lc);

  // go to disk since we might not have all the metrics in memory
  std::vector<std::string> res;
  for(boost::filesystem::recursive_directory_iterator end, cur(_path + "/"); cur != end; ++cur) {
      // we are looking for files
      if((*cur).status().type() != boost::filesystem::regular_file) {
          continue;
      }

      // with specified extension
      boost::filesystem::path file_path((*cur).path());
      if(file_path.extension() != RRDB_METRIC_EXTENSION) {
          continue;
      }

      // cheat here - we know names
      std::string name = file_path.stem();
      if(!like_lc.empty() &&  name.find(like_lc) == std::string::npos) {
          continue;
      }

      // found!
      res.push_back(name);
  }

  // done
  return res;
}

rrdb::t_metrics_vector rrdb::get_dirty_metrics()
{
  // lock access to _metrics
  rrdb::t_metrics_vector res;
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    BOOST_FOREACH(t_metrics_map::value_type & v, _metrics){
      if(v.second->is_dirty()) {
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
