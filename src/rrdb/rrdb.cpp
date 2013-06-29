/*
 * rrdb.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#include <sstream>

#include <boost/thread/locks.hpp>
#include <boost/foreach.hpp>


#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric.h"
#include "rrdb/rrdb_files_cache.h"
#include "rrdb/rrdb_journal_file.h"
#include "rrdb/rrdb_metric_tuples_cache.h"

#include "parser/interval.h"
#include "parser/statements.h"

#include "server/server.h"

#include "common/config.h"
#include "common/log.h"
#include "common/exception.h"


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
     metrics_walker_show_metrics(t_memory_buffer & res) :
       _res(res)
     {
     }

     virtual ~metrics_walker_show_metrics()
     {
     }

   public:
     //rrdb::metrics_walker
     void on_metric(const std::string & name, const boost::intrusive_ptr<rrdb_metric> & metric)
     {
       _res << name  << std::endl;
     }
   private:
     mutable t_memory_buffer &  _res;
   }; // class metrics_walker_show_metrics

  //
  // Walker class for SHOW STATUS statement
  //
  class metrics_walker_show_status :
       public rrdb::metrics_walker
  {
   public:
    metrics_walker_show_status(t_memory_buffer & res) :
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
     void on_metric(const std::string & name, const boost::intrusive_ptr<rrdb_metric> & metric)
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
     mutable t_memory_buffer &  _res;
     my::value_t                _value;
     my::time_t                 _value_ts;
   }; // class metrics_walker_show_status

  //
  // Walker class for SELECT statement w/o group by
  //
  class data_walker_select_no_group_by :
      public rrdb::data_walker
  {
  public:
    data_walker_select_no_group_by(const statement_select & select, t_memory_buffer & res) :
      _select(select),
      _res(res),
      _last_ts(select._ts_end + 1)
    {
    }

    virtual ~data_walker_select_no_group_by()
    {

    }


  public:
    // rrdb::data_walker
    void append(const t_rrdb_metric_tuple & tuple, const my::interval_t & interval)
    {
      // we are guaranteed that tuple is in the select's [ts1, ts2) interval
      // but do we want this tuple? we might have lower resolution data in subsequent
      // blocks so we just use our last_ts to figure it out
      if(_select._ts_begin <= tuple._ts && tuple._ts < _last_ts && tuple._count > 0) {
        rrdb_metric_tuple_write(tuple, _res);
        _last_ts = tuple._ts;
      }
    }

    virtual void flush()
    {
      // do nothing - we write immediately
    }

  private:
    const statement_select  & _select;
    mutable t_memory_buffer & _res;
    my::time_t                _last_ts;
  }; // class data_walker_select_no_group_by

  //
  // Walker class for SELECT statement
  //
  class data_walker_select :
        public rrdb::data_walker
    {
    public:
      data_walker_select(const statement_select & select, t_memory_buffer & res) :
        _select(select),
        _res(res),
        _group_by(*select._group_by),
        _ts(0),
        _ts_beg(0),
        _ts_end(0)
      {
        CHECK_AND_THROW(_group_by > 0);
        CHECK_AND_THROW(select._ts_begin < select._ts_end);

        LOG(log::LEVEL_INFO, "Start select with group by %lu", _group_by);

        // we want to put the "end" of the first interval at
        //
        //      select._ts_begin + N*_group_by >= select._ts_end
        //
        // for the smallest possible N
        my::time_t x = (select._ts_end - select._ts_begin) % _group_by;
        if(x > 0) {
            this->reset_cur_tuple(select._ts_end + _group_by - x);
        } else {
            this->reset_cur_tuple(select._ts_end);
        }
      }

      virtual ~data_walker_select()
      {
      }

    public:
      // rrdb::data_walker
      void append(const t_rrdb_metric_tuple & tuple, const my::interval_t & interval)
      {
        // LOG(log::LEVEL_DEBUG3, "cur ts: %lu, tuple ts: %lu, interval: %lu, count: %f", _ts, tuple._ts, interval, tuple._count);

        // while there is no overlap yet (i.e. [tuple) < [_ts_beg, _ts_end) )
        // flush tuple and move to the next one
        my::time_t tuple_end = tuple._ts + interval;
        while(tuple_end <= _ts_beg) {
            this->flush();
            this->reset_cur_tuple(_ts_beg);
        }

        // check if tuple is newer than "group by" interval (for example from lower resolution
        // blocks stuffed in later). otherwise we will get duplicate data
        while(tuple._ts < _ts && _select._ts_begin < _ts) {
            // there is an overlap since tuple.b < _ts <= _ts_end && _ts_beg < _ts <= tuple.e
            my::time_t lower_bound = std::max(tuple._ts, _ts_beg);
            my::time_t upper_bound = std::min(tuple_end, _ts);
            my::time_t overlap = upper_bound - lower_bound;
            if(_group_by < overlap) {
                overlap = _group_by;
            }

            // check if tuple is "inside" the the current "group by" interval
            if(overlap >= interval) {
                rrdb_metric_tuple_update(_cur_tuple, tuple);
            } else if(overlap > 0) {
                // calculate the contribution of the tuple based
                // simple proportion
                rrdb_metric_tuple_update(_cur_tuple, tuple, overlap / (double)interval);
            }

            // are we done with this tuple?
            if(_ts_beg <= tuple._ts) {
                _ts = tuple._ts;
                break;
            }

            // more "group by" intervals that fit this tuple
            this->flush();
            this->reset_cur_tuple(_ts_beg);
        }
      }

      virtual void flush()
      {
        if(_cur_tuple._count > 0) {
            rrdb_metric_tuple_write(_cur_tuple, _res);
        }
      }

    private:
      void reset_cur_tuple(const my::time_t & new_ts_end)
      {
         // shift the interval, initially _ts is set to the _ts_end
         _ts     = new_ts_end;
         _ts_end = new_ts_end;
         _ts_beg = std::max(new_ts_end - _group_by, _select._ts_begin);

          // reset the tuple, tuple starts at _beg of the interval
          memset(&_cur_tuple, 0, sizeof(_cur_tuple));
          _cur_tuple._ts = _ts_beg;
      }

    private:
      const statement_select  & _select;
      mutable t_memory_buffer & _res;
      const my::interval_t      _group_by;

      t_rrdb_metric_tuple       _cur_tuple;
      my::time_t                _ts;
      my::time_t                _ts_beg;
      my::time_t                _ts_end;
    }; // class data_walker


public:
  statement_execute_visitor(rrdb & rrdb, t_memory_buffer & res) :
      _rrdb(rrdb),
      _res(res)
  {
  }

public:
  void operator()(const statement_create & st) const
  {
    if(st._policy) {
        _rrdb.create_metric(st._name, st._policy.get());
    } else {
        _rrdb.create_metric(st._name, _rrdb.get_default_policy());
    }
  }

  void operator()(const statement_drop & st) const
  {
    _rrdb.drop_metric(st._name);
  }

  void operator()(const statement_update & st) const
  {
    _rrdb.update_metric(st._name, st._ts ? *st._ts : time(NULL), st._value);
  }

  void operator()(const statement_select & st) const
  {
    // write header
    rrdb_metric_tuple_write_header(_res);

    if(st._ts_begin < st._ts_end) {
      if(st._group_by && (*st._group_by)) {
          // hard case
          data_walker_select walker(st, _res);
          _rrdb.select_from_metric(st._name, st._ts_begin, st._ts_end, walker);
      } else {
          // simple case
          data_walker_select_no_group_by walker(st, _res);
          _rrdb.select_from_metric(st._name, st._ts_begin, st._ts_end, walker);
      }
    }
  }

  void operator()(const statement_show_policy & st) const
  {
    boost::intrusive_ptr<rrdb_metric> metric = _rrdb.get_metric(st._name);
    _res << retention_policy_write(metric->get_policy());

  }

  void operator()(const statement_show_metrics & st) const
  {
    metrics_walker_show_metrics walker(_res);
    _rrdb.get_metrics(st._like, walker);
  }

  void operator()(const statement_show_status & st) const
  {
    metrics_walker_show_status walker(_res);
    _rrdb.get_status_metrics(st._like, walker);

  }

private:
  mutable rrdb & _rrdb;
  mutable t_memory_buffer & _res;
};
// statement_execute_visitor

//
//
//
rrdb::rrdb() :
  _flush_interval(interval_parse("1 min")),
  _default_policy(retention_policy_parse("1 min FOR 1 day"))
{
  _files_cache.reset(new rrdb_files_cache());
  _tuples_cache.reset(new rrdb_metric_tuples_cache(_files_cache));
  _journal_file.reset(new rrdb_journal_file(_files_cache));
}

rrdb::~rrdb()
{
  this->stop();
}

void rrdb::initialize(boost::shared_ptr<config> config)
{
  _flush_interval = interval_parse(
      config->get<std::string>("rrdb.flush_interval", interval_write(_flush_interval))
  );
  _default_policy = retention_policy_parse(
      config->get<std::string>("rrdb.default_policy", retention_policy_write(_default_policy))
  );

  LOG(log::LEVEL_DEBUG, "Loading RRDB data files");
  _files_cache->initialize(config);
  _tuples_cache->initialize(config);
  _journal_file->initialize();

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

  LOG(log::LEVEL_INFO, "Starting RRDB server");

  // check if there was a crash
  if(_journal_file->is_journal_file_present()) {
      LOG(log::LEVEL_INFO, "Found journal file '%s', starting recovery", _journal_file->get_jnl_full_path().c_str());

      _journal_file->load_journal_file();
      LOG(log::LEVEL_INFO, "Loaded journal file '%s'", _journal_file->get_jnl_full_path().c_str());

      // write data to the metric file
      my::filename_t filename(new std::string(_journal_file->get_filename()));
      _journal_file->apply_journal(filename);
      LOG(log::LEVEL_INFO, "Applied journal file '%s' to file '%s'", _journal_file->get_jnl_full_path().c_str(), filename->c_str());
  }

  // load metrics from disk - we do it under lock though it doesn't matter
  {
    LOG(log::LEVEL_INFO, "Loading metrics");
    boost::lock_guard<spinlock> guard(_metrics_lock);

    rrdb_metric::create_directories(_files_cache->get_path());
    rrdb_metric::load_metrics(_files_cache, _files_cache->get_path(), _metrics);
    LOG(log::LEVEL_INFO, "Loaded metrics");
  }




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

  //
  _files_cache->clear();
  _tuples_cache->clear();

  LOG(log::LEVEL_INFO, "Stopped RRDB server");
}


void rrdb::update_status(const time_t & now)
{
  this->update_metric("self.file_cache.max_size", now,   _files_cache->get_max_size());
  this->update_metric("self.file_cache.size", now,       _files_cache->get_cache_size());
  this->update_metric("self.file_cache.hits", now,       _files_cache->get_cache_hits(true));
  this->update_metric("self.file_cache.misses", now,     _files_cache->get_cache_misses(true));

  this->update_metric("self.blocks_cache.max_used_memory", now,   _tuples_cache->get_max_used_memory());
  this->update_metric("self.blocks_cache.used_memory", now,       _tuples_cache->get_cache_used_memory());
  this->update_metric("self.blocks_cache.size", now,              _tuples_cache->get_cache_size());
  this->update_metric("self.blocks_cache.hits", now,              _tuples_cache->get_cache_hits(true));
  this->update_metric("self.blocks_cache.misses", now,            _tuples_cache->get_cache_misses(true));

  this->update_metric("self.metrics.count", now, this->get_metrics_num());
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
  BOOST_FOREACH(boost::intrusive_ptr<rrdb_metric> metric, dirty_metrics) {
    try {
        // write metric data into the journal's internal buffer
        metric->save_dirty_blocks(_files_cache, _journal_file);

        // write data to the journal file
        _journal_file->save_journal_file();

        // write data to the metric's file - we expect the metric file to already exist
        _journal_file->apply_journal(metric->get_filename());

        // check if metric was deleted meantime and force delete it again
        if(metric->is_deleted()) {
            metric->delete_file(_files_cache, _tuples_cache);
        }
    } catch(std::exception & e) {
        LOG(log::LEVEL_ERROR, "Exception saving metric '%s': %s", metric->get_name().c_str(), e.what());
    } catch(...) {
        LOG(log::LEVEL_ERROR, "Unknown exception saving metric '%s'", metric->get_name().c_str());
    }

    // always reset journal
    _journal_file->clear();
  }

  // we are don - journal file no more
  _journal_file->delete_journal_file();

  LOG(log::LEVEL_DEBUG2, "Flushed to disk");
}

my::size_t rrdb::get_metrics_num() const
{
  boost::lock_guard<spinlock> guard(_metrics_lock);
  return _metrics.size();
}

boost::intrusive_ptr<rrdb_metric> rrdb::find_metric(const std::string & name)
{
  // normalize name
  std::string name_normalized(rrdb_metric::normalize_name(name));

  // search in the map: lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    t_metrics_map::const_iterator it = _metrics.find(name_normalized);
    if(it != _metrics.end()) {
        return (*it).second;
    }
  }
  return boost::intrusive_ptr<rrdb_metric>();
}

boost::intrusive_ptr<rrdb_metric> rrdb::get_metric(const std::string & name)
{
  boost::intrusive_ptr<rrdb_metric> res = this->find_metric(name);
  if(!res) {
      throw exception("The metric '%s' does not exist", name.c_str());
  }
  return res;
}

boost::intrusive_ptr<rrdb_metric> rrdb::create_metric(const std::string & name, const t_retention_policy & policy, bool throw_if_exists)
{
  // log
  LOG(log::LEVEL_DEBUG, "RRDB: creating metric '%s' with policy '%s'", name.c_str(), retention_policy_write(policy).c_str());

  // normalize the name
  std::string name_normalized(rrdb_metric::normalize_name(name));

  // check the policy
  retention_policy_validate(policy);

  // create new and try to insert into map, lock access to _metrics
  boost::intrusive_ptr<rrdb_metric> res(new rrdb_metric());
  res->create(name_normalized, policy);
  {
    // make sure there is always only one metric for the name
    boost::lock_guard<spinlock> guard(_metrics_lock);
    t_metrics_map::const_iterator it = _metrics.find(name_normalized);
    if(it != _metrics.end()) {
        // someone inserted it in the meantime
        if(throw_if_exists) {
            throw exception("The metric '%s' already exists", name.c_str());
        } else {
            return (*it).second;
        }
    }
    _metrics[name_normalized] = res;
  }

  // write to disk
  res->save_file(this->get_files_cache());

  // log
  LOG(log::LEVEL_INFO, "RRDB: created metric '%s' with policy '%s'", name.c_str(), retention_policy_write(policy).c_str());

  return res;
}

void rrdb::drop_metric(const std::string & name)
{
  // log
  LOG(log::LEVEL_DEBUG, "RRDB: dropping metric '%s'", name.c_str());

  // normalize name
  std::string name_normalized(rrdb_metric::normalize_name(name));

  // lock access to _metrics and  try to find the metric
  boost::intrusive_ptr<rrdb_metric> res;
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    t_metrics_map::const_iterator it = _metrics.find(name_normalized);
    if(it == _metrics.end()) {
        throw exception("The metric '%s' does not exists", name.c_str());
    }

    res = (*it).second;
    _metrics.erase(it);
  }

  // delete file - outside the spin lock
  res->delete_file(this->get_files_cache(), this->get_tuples_cache());

  // log
  LOG(log::LEVEL_INFO, "RRDB: dropped metric '%s'", name.c_str());
}

void rrdb::get_metrics(const boost::optional<std::string> & like, metrics_walker & walker)
{
  // normalize the like (same as name)
  boost::optional<std::string> like_normalized;
  if(like) {
      like_normalized = rrdb_metric::normalize_name(*like);
  }

  // lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    BOOST_FOREACH(const t_metrics_map::value_type & v, _metrics) {
      if(like_normalized && v.first.find(*like_normalized) == std::string::npos) {
          continue;
      }
      walker.on_metric(v.first, v.second);
    }
  }
}

void rrdb::get_status_metrics(const boost::optional<std::string> & like, metrics_walker & walker)
{
  // normalize the like (same as name)
  boost::optional<std::string> like_normalized;
  if(like) {
      like_normalized = rrdb_metric::normalize_name(*like);
  }

  // lock access to _metrics
  {
    boost::lock_guard<spinlock> guard(_metrics_lock);
    BOOST_FOREACH(const t_metrics_map::value_type & v, _metrics) {
      // should start with "self."
      if(v.first.find("self.") != 0) {
          continue;
      }
      if(like_normalized && v.first.find(*like_normalized) == std::string::npos) {
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
  boost::intrusive_ptr<rrdb_metric> metric = this->find_metric(name);
  if(!metric) {
      metric = this->create_metric(name, _default_policy, false);
  }

  metric->update(_tuples_cache, ts, value);
}

void rrdb::select_from_metric(const std::string & name, const my::time_t & ts1, const my::time_t & ts2, data_walker & walker)
{
  boost::intrusive_ptr<rrdb_metric> metric = this->find_metric(name);
  if(!metric) {
      throw exception("The metric '%s' does not exist", name.c_str());
  }

  metric->select(_tuples_cache, ts1, ts2, walker);
}

void rrdb::execute_query_statement(const std::string & buffer, t_memory_buffer & res)
{
  LOG(log::LEVEL_DEBUG3, "TCP command: '%s'", buffer.c_str());

  t_statement st = statement_query_parse(buffer);
  boost::apply_visitor<>(statement_execute_visitor(*this, res), st);
}

void rrdb::execute_update_statement(const std::string & buffer, t_memory_buffer & res)
{
  LOG(log::LEVEL_DEBUG3, "UDP command: %s", buffer.c_str());

  t_statement st = statement_update_parse(buffer);
  boost::apply_visitor<>(statement_execute_visitor(*this, res), st);
}

