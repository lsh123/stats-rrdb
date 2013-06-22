/*
 * rrdb.h
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#ifndef RRDB_H_
#define RRDB_H_

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

#include "common/types.h"
#include "common/spinlock.h"
#include "common/memory_buffer.h"

#include "parser/interval.h"
#include "parser/retention_policy.h"

#include "rrdb/rrdb_metric_tuple.h"

// forward
class rrdb_metric;
class rrdb_file_cache;
class rrdb_metric_tuples_cache;

class config;
class statement_select;
class server;

class rrdb :
    public boost::enable_shared_from_this<rrdb>
{
public:
  typedef boost::unordered_map< std::string, boost::shared_ptr<rrdb_metric> > t_metrics_map;
  typedef std::vector< boost::shared_ptr<rrdb_metric> > t_metrics_vector;

  //
  // Walks through metrics
  //
  class metrics_walker {
  public:
    virtual void on_metric(const std::string & name, const boost::shared_ptr<rrdb_metric> & metric) = 0;
  }; // class metrics_walker

  //
  // Walks through the data
  //
  class data_walker {
  public:
    virtual void append(const t_rrdb_metric_tuple & tuple, const my::interval_t & interval) = 0;
    virtual void flush() = 0;
  }; // class data_walker

public:
  rrdb();
  virtual ~rrdb();

  void initialize(boost::shared_ptr<config> config);

  bool is_running();
  void start();
  void stop();

  void update_status(const time_t & now);

  // metrics map operations
  boost::shared_ptr<rrdb_metric> get_metric(const std::string & name);
  boost::shared_ptr<rrdb_metric> find_metric(const std::string & name);

  boost::shared_ptr<rrdb_metric> create_metric(const std::string & name, const t_retention_policy & policy, bool throw_if_exists = true);
  void drop_metric(const std::string & name);

  void get_metrics(const boost::optional<std::string> & like, metrics_walker & walker);
  void get_status_metrics(const boost::optional<std::string> & like, metrics_walker & walker);

  // values
  void update_metric(const std::string & name, const my::time_t & ts, const my::value_t & value);
  void select_from_metric(const std::string & name, const my::time_t & ts1, const my::time_t & ts2, data_walker & walker);

  // commands
  void execute_tcp_command(const std::string & buffer, t_memory_buffer & res);
  void execute_udp_command(const std::string & buffer, t_memory_buffer & res);

  // helpers
  const boost::shared_ptr<rrdb_file_cache> & get_files_cache() const
  {
    return _files_cache;
  }
  const boost::shared_ptr<rrdb_metric_tuples_cache> & get_tuples_cache() const
  {
    return _blocks_cache;
  }

private:
  boost::shared_ptr<rrdb_metric> find_metric_lc(const std::string & name_lc);

  void flush_to_disk_thread();
  void flush_to_disk();
  t_metrics_vector get_dirty_metrics();

private:
  // config
  my::interval_t          _flush_interval;
  t_retention_policy        _default_policy;

  t_metrics_map           _metrics;
  spinlock                _metrics_lock;

  boost::shared_ptr<rrdb_file_cache>         _files_cache;
  boost::shared_ptr<rrdb_metric_tuples_cache> _blocks_cache;
  boost::shared_ptr< boost::thread >         _flush_to_disk_thread;
}; // class rrdb

#endif /* RRDB_H_ */
