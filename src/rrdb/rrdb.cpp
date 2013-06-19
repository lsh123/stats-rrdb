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

#include "server/server.h"

#include "config.h"
#include "log.h"
#include "exception.h"


//
//
//
class statement_execute_visitor : public boost::static_visitor<void>
{
  //
  // Walker class for SHOW METRICS statement
  //
  class metrics_walker_show_metrics :
       public rrdb::metrics_walker
   {
   public:
     metrics_walker_show_metrics(memory_buffer_t & res) :
       _res(res)
     {
     }

     virtual ~metrics_walker_show_metrics()
     {
     }

   public:
     //rrdb::metrics_walker
     void on_metric(const std::string & name, const boost::shared_ptr<rrdb_metric> & metric)
     {
       _res << name  << std::endl;
     }
   private:
     mutable memory_buffer_t &  _res;
   }; // class metrics_walker_show_metrics

  //
  // Walker class for SHOW STATUS statement
  //
  class metrics_walker_show_status :
       public rrdb::metrics_walker
   {
   public:
    metrics_walker_show_status(memory_buffer_t & res) :
       _res(res),
       _value(0),
       _value_ts(0)
     {
     }

     virtual ~metrics_walker_show_status()
     {
     }

   public:
     //rrdb::metrics_walker
     void on_metric(const std::string & name, const boost::shared_ptr<rrdb_metric> & metric)
     {

       metric->get_last_value(_value, _value_ts);

       _res << name
            <<  ','
            << _value_ts
            << ','
            << _value
            << std::endl
       ;
     }
   private:
     mutable memory_buffer_t &  _res;
     my::value_t                _value;
     my::time_t                 _value_ts;
   }; // class metrics_walker_show_status


public:
  statement_execute_visitor(const boost::shared_ptr<rrdb> rrdb, memory_buffer_t & res) :
      _rrdb(rrdb),
      _res(res)
  {
  }

public:
  void operator()(const statement_create & st) const
  {
    _rrdb->create_metric(st._name, st._policy);
  }

  void operator()(const statement_drop & st) const
  {
    _rrdb->drop_metric(st._name);
  }

  void operator()(const statement_update & st) const
  {
    _rrdb->update_metric(st._name, st._ts ? *st._ts : time(NULL), st._value);
  }

  void operator()(const statement_select & st) const
  {
    std::vector<rrdb_metric_tuple_t> tuples;
    _rrdb->select_from_metric(st, tuples);

    _res << "ts,count,sum,avg,stddev,min,max" << std::endl;
    my::value_t avg, stddev;
    BOOST_FOREACH(const rrdb_metric_tuple_t & tuple, tuples) {
      if(tuple._count > 0) {
          avg = tuple._sum / tuple._count;
          stddev = sqrt(tuple._sum_sqr / tuple._count - avg * avg) ;
      } else {
          avg = stddev = 0;
      }
      _res << tuple._ts
           << ','
           << tuple._count
           << ','
           << tuple._sum
           << ','
           << avg
           << ','
           << stddev
           << ','
           << tuple._min
           << ','
           << tuple._max
           << std::endl;
      ;
    }
  }

  void operator()(const statement_show_policy & st) const
  {
    boost::shared_ptr<rrdb_metric> metric = _rrdb->get_metric(st._name);
    _res << retention_policy_write(metric->get_policy());

  }

  void operator()(const statement_show_metrics & st) const
  {
    metrics_walker_show_metrics walker(_res);
    _rrdb->get_metrics(st._like, walker);
  }

  void operator()(const statement_show_status & st) const
  {
    metrics_walker_show_status walker(_res);
    _rrdb->get_status_metrics(st._like, walker);

  }

private:
  const boost::shared_ptr<rrdb> _rrdb;
  mutable memory_buffer_t &  _res;
};
// statement_execute_visitor

//
//
//
rrdb::rrdb(boost::shared_ptr<server> server) :
  _server(server),
  _path("/var/lib/rrdb"),
  _flush_interval(interval_parse("1 min")),
  _default_policy(retention_policy_parse("1 min FOR 1 day"))

{
}

rrdb::~rrdb()
{
  this->stop();
}

void rrdb::initialize(boost::shared_ptr<config> config)
{
  _path           = config->get<std::string>("rrdb.path", _path);

  _flush_interval = interval_parse(
      config->get<std::string>("rrdb.flush_interval", interval_write(_flush_interval))
  );
  _default_policy = retention_policy_parse(
      config->get<std::string>("rrdb.default_policy", retention_policy_write(_default_policy))
  );


  LOG(log::LEVEL_DEBUG, "Loading RRDB data files");

  // load metrics from disk
  this->load_metrics();

  LOG(log::LEVEL_INFO, "Loaded RRDB data files");
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

  LOG(log::LEVEL_DEBUG, "Starting RRDB server");

  // start flush thread
  _flush_to_disk_thread.reset(new boost::thread(boost::bind(&rrdb::flush_to_disk_thread, this)));

  // done
  LOG(log::LEVEL_INFO, "Started RRDB server");
}

void rrdb::stop()
{
  if(!this->is_running()) {
      return;
  }

  LOG(log::LEVEL_DEBUG, "Stopping RRDB server");

  // stop flush thread
  _flush_to_disk_thread->interrupt();
  _flush_to_disk_thread->join();
  _flush_to_disk_thread.reset();

  // flush one more time
  this->flush_to_disk();

  LOG(log::LEVEL_INFO, "Stopped RRDB server");
}


void rrdb::update_status(const time_t & now)
{
  // TODO
}

void rrdb::flush_to_disk_thread()
{
  // log
  LOG(log::LEVEL_INFO, "RRDB Flush thread started");

  // try/catch to get any error reported
  try {
      while (!boost::this_thread::interruption_requested()) {
          time_t start = time(NULL);
          {
            boost::this_thread::disable_interruption d;

            this->flush_to_disk();
          }
          // eat our own dog food
          time_t end = time(NULL);
          this->update_metric("self.flush_to_disk.duration", end, (end - start));


          boost::this_thread::sleep(boost::posix_time::seconds(this->_flush_interval));
      }
  } catch (boost::thread_interrupted & e) {
      LOG(log::LEVEL_DEBUG, "RRDB Flush thread was interrupted");
  } catch (std::exception & e) {
      LOG(log::LEVEL_ERROR, "RRDB Flush thread exception: %s", e.what());
      throw e;
  } catch (...) {
      LOG(log::LEVEL_ERROR, "RRDB Flush thread un-handled exception");
      throw;
  }

  // done
  LOG(log::LEVEL_INFO, "RRDB Flush thread stopped");
}

void rrdb::flush_to_disk()
{
  LOG(log::LEVEL_DEBUG2, "Flushing to disk");

  t_metrics_vector dirty_metrics = this->get_dirty_metrics();
  BOOST_FOREACH(boost::shared_ptr<rrdb_metric> metric, dirty_metrics) {
    try {
        metric->save_file(_path);
    } catch(std::exception & e) {
      LOG(log::LEVEL_ERROR, "Exception saving metric '%s': %s", metric->get_name().c_str(), e.what());
    } catch(...) {
        LOG(log::LEVEL_ERROR, "Unknown exception saving metric '%s'", metric->get_name().c_str());
    }
  }

  LOG(log::LEVEL_INFO, "Flushed to disk");
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
      LOG(log::LEVEL_DEBUG3, "Checking file %s", (*cur).path().string().c_str());

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

boost::shared_ptr<rrdb_metric> rrdb::create_metric(const std::string & name, const retention_policy & policy, bool throw_if_exists)
{
  // log
  LOG(log::LEVEL_DEBUG, "RRDB: creating metric '%s' with policy '%s'", name.c_str(), retention_policy_write(policy).c_str());

  // force lower case for names
  std::string name_lc(name);
  boost::algorithm::to_lower(name_lc);

  // TODO: check name
  // starts with a letter, doesn't contain anything but (a-zA-Z0-9._-), length

  // check the policy
  retention_policy_validate(policy);

  // create new and try to insert into map, lock access to _metrics
  boost::shared_ptr<rrdb_metric> res(new rrdb_metric(name_lc, policy));
  res->set_dirty();
  {
    // make sure there is always only one metric for the name
    boost::lock_guard<spinlock> guard(_metrics_lock);
    t_metrics_map::const_iterator it = _metrics.find(name_lc);
    if(it != _metrics.end()) {
        // someone inserted it in the meantime
        if(throw_if_exists) {
            throw exception("The metric '%s' already exists", name.c_str());
        } else {
            return (*it).second;
        }
    }
    _metrics[name_lc] = res;
  }

  // log
  LOG(log::LEVEL_INFO, "RRDB: created metric '%s' with policy '%s'", name.c_str(), retention_policy_write(policy).c_str());

  return res;
}

void rrdb::drop_metric(const std::string & name)
{
  // log
  LOG(log::LEVEL_DEBUG, "RRDB: dropping metric '%s'", name.c_str());

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
  LOG(log::LEVEL_INFO, "RRDB: dropped metric '%s'", name.c_str());
}

void rrdb::get_metrics(const boost::optional<std::string> & like, metrics_walker & walker)
{
  // force lower case for names
  boost::optional<std::string> like_lc(like);
  if(like_lc) {
      boost::algorithm::to_lower(*like_lc);
  }

  // lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    BOOST_FOREACH(const t_metrics_map::value_type & v, _metrics) {
      if(like_lc && v.first.find(*like_lc) == std::string::npos) {
          continue;
      }
      walker.on_metric(v.first, v.second);
    }
  }
}

void rrdb::get_status_metrics(const boost::optional<std::string> & like, metrics_walker & walker)
{
  // force lower case for names
  boost::optional<std::string> like_lc(like);
  if(like_lc) {
      boost::algorithm::to_lower(*like_lc);
  }

  // lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    BOOST_FOREACH(const t_metrics_map::value_type & v, _metrics) {
      // should start with "self."
      if(v.first.find("self.") != 0) {
          continue;
      }
      if(like_lc && v.first.find(*like_lc) == std::string::npos) {
          continue;
      }
      walker.on_metric(v.first, v.second);
    }
  }
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
void rrdb::update_metric(const std::string & name, const my::time_t & ts, const my::value_t & value)
{
  boost::shared_ptr<rrdb_metric> metric = this->find_metric(name);
  if(!metric) {
      metric = this->create_metric(name, _default_policy, false);
  }

  metric->update(ts, value);
}

void rrdb::select_from_metric(const statement_select & query, std::vector<rrdb_metric_tuple_t> & res)
{
  boost::shared_ptr<rrdb_metric> metric = this->find_metric(query._name);
  if(!metric) {
      // TODO: create metric automatically when needed
      throw exception("The metric '%s' does not exist", query._name.c_str());
  }

  metric->select(query, res);
}

void rrdb::execute_tcp_command(const std::vector<char> & buffer, memory_buffer_t & res)
{
  // LOG(log::LEVEL_DEBUG3, "RRDB command: '%s'", std::string(buffer.begin(), buffer.end()).c_str());

  statement st = statement_parse_tcp(buffer.begin(), buffer.end());
  boost::apply_visitor<>(statement_execute_visitor(shared_from_this(), res), st);
}

void rrdb::execute_udp_command(const std::string & buffer, memory_buffer_t & res)
{
  // LOG(log::LEVEL_DEBUG3, "Short command: %s", buffer.c_str());

  statement st = statement_parse_udp(buffer);
  boost::apply_visitor<>(statement_execute_visitor(shared_from_this(), res), st);
}

