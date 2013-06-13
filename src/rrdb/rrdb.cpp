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

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric.h"

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

  std::string operator()(const statement_update & st) const
  {
    _rrdb->update_metric(st._name, st._ts, st._value);
    return "OK";
  }

  std::string operator()(const statement_select & st) const
  {
    std::vector<rrdb_metric_tuple_t> tuples;
    _rrdb->select_from_metric(st._name, st._ts_begin, st._ts_end, tuples);

    std::ostringstream res;
    res << "ts,count,sum,sum_sqr,min,max" << std::endl;
    BOOST_FOREACH(const rrdb_metric_tuple_t & tuple, tuples) {
      res << tuple._ts
          << ','
          << tuple._count
          << ','
          << tuple._sum
          << ','
          << tuple._sum_sqr
          << ','
          << tuple._min
          << ','
          << tuple._max
          << std::endl;
      ;
    }
    return res.str();
  }

  std::string operator()(const statement_show_policy & st) const
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

  // load metrics from disk
  this->load_metrics();

  // start flush thread
  _flush_to_disk_thread.reset(new boost::thread(boost::bind(&rrdb::flush_to_disk_thread, this)));

  // done
  log::write(log::LEVEL_INFO, "Started RRDB server");
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
  log::write(log::LEVEL_DEBUG2, "Flushing to disk");

  t_metrics_vector dirty_metrics = this->get_dirty_metrics();
  BOOST_FOREACH(boost::shared_ptr<rrdb_metric> metric, dirty_metrics) {
    try {
        metric->save_file(_path);
    } catch(std::exception & e) {
      log::write(log::LEVEL_ERROR, "Exception saving metric '%s': %s", metric->get_name().c_str(), e.what());
    }
  }

  log::write(log::LEVEL_DEBUG2, "Flushed to disk");
}

boost::shared_ptr<rrdb_metric> rrdb::find_metric(const std::string & name)
{
  // force lower case for names
  std::string name_lc(name);
  boost::algorithm::to_lower(name_lc);

  return this->find_metric_lc(name_lc);
}

void rrdb::load_metrics()
{
  // ensure folders exist
  boost::filesystem::create_directories(_path);

  for(boost::filesystem::recursive_directory_iterator end, cur(_path + "/"); cur != end; ++cur) {
      log::write(log::LEVEL_DEBUG3, "Checking file %s", (*cur).path().string().c_str());

      // we are looking for files
      if((*cur).status().type() != boost::filesystem::regular_file) {
          continue;
      }

      // with specified extension
      boost::filesystem::path file_path((*cur).path());
      if(file_path.extension() != RRDB_METRIC_EXTENSION) {
          continue;
      }

      // load metric
      boost::shared_ptr<rrdb_metric> metric(rrdb_metric::load_file(file_path.string()));
      std::string name(metric->get_name());

      // try to insert into the map
      {
        boost::lock_guard<spinlock> guard(_metrics_lock);
        t_metrics_map::const_iterator it = _metrics.find(name);
        if(it == _metrics.end()) {
            // insert loaded metric into map and return
            _metrics[name] = metric;
        }
      }
  }
}

boost::shared_ptr<rrdb_metric> rrdb::find_metric_lc(const std::string & name_lc)
{
  // search in the map: lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    t_metrics_map::const_iterator it = _metrics.find(name_lc);
    if(it != _metrics.end()) {
        return (*it).second;
    }
  }
  return boost::shared_ptr<rrdb_metric>();
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

  // store results here
  std::vector<std::string> res;

  // lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    BOOST_FOREACH(const t_metrics_map::value_type & v, _metrics) {
      if(v.first.find(like_lc) == std::string::npos) {
          continue;
      }
      res.push_back(v.first);
    }
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

// values
void rrdb::update_metric(const std::string & name, const boost::uint64_t & ts, const double & value)
{
  boost::shared_ptr<rrdb_metric> metric = this->find_metric(name);
  if(!metric) {
      metric = this->create_metric(name, _default_policy);
  }

  metric->update(ts, value);
}

void rrdb::select_from_metric(const std::string & name, const boost::uint64_t & ts_begin, const boost::uint64_t & ts_end, std::vector<rrdb_metric_tuple_t> & res)
{
  boost::shared_ptr<rrdb_metric> metric = this->find_metric(name);
  if(!metric) {
      // TODO: create metric automatically when needed
      throw exception("The metric '%s' does not exist", name.c_str());
  }

  metric->select(ts_begin, ts_end, res);
}


rrdb::t_result_buffers rrdb::execute_long_command(const std::vector<char> & buffer)
{
  // log::write(log::LEVEL_DEBUG, "RRDB command: '%s'", std::string(buffer.begin(), buffer.end()).c_str());

  statement st = statement_parse(buffer.begin(), buffer.end());
  std::string output = boost::apply_visitor(statement_execute_visitor(shared_from_this()), st);

  rrdb::t_result_buffers res;
  res.push_back(boost::asio::buffer(output));
  return res;
}

void rrdb::execute_short_command(const std::vector<char> & buffer)
{

}
