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

#include "types.h"
#include "spinlock.h"
#include "memory_buffer.h"

#include "parser/interval.h"
#include "parser/retention_policy.h"

#include "rrdb/rrdb_metric_tuple.h"

// forward
class config;
class rrdb_metric;
class statement_select;
class server;

class rrdb :
    public boost::enable_shared_from_this<rrdb>
{
public:
  typedef boost::unordered_map< std::string, boost::shared_ptr<rrdb_metric> > t_metrics_map;
  typedef std::vector< boost::shared_ptr<rrdb_metric> > t_metrics_vector;

public:
  rrdb(boost::shared_ptr<server> server);
  virtual ~rrdb();

  void initialize(boost::shared_ptr<config> config);

  bool is_running();
  void start();
  void stop();

  void update_status(const time_t & now);

  // metrics map operations
  boost::shared_ptr<rrdb_metric> get_metric(const std::string & name);
  boost::shared_ptr<rrdb_metric> find_metric(const std::string & name);
  boost::shared_ptr<rrdb_metric> create_metric(const std::string & name, const retention_policy & policy, bool throw_if_exists = true);
  void drop_metric(const std::string & name);
  void get_metrics(const boost::optional<std::string> & like, std::vector<std::string> & res);
  void get_status_metrics(const boost::optional<std::string> & like, std::vector<std::string> & res);

  // values
  void update_metric(const std::string & name, const my::time_t & ts, const my::value_t & value);
  void select_from_metric(const statement_select & query, std::vector<rrdb_metric_tuple_t> & res);

  // commands
  void execute_tcp_command(const std::vector<char> & buffer, memory_buffer_t & res);
  void execute_udp_command(const std::string & buffer, memory_buffer_t & res);

private:
  void load_metrics();
  boost::shared_ptr<rrdb_metric> find_metric_lc(const std::string & name_lc);

  void flush_to_disk_thread();
  void flush_to_disk();
  t_metrics_vector get_dirty_metrics();

private:
  boost::weak_ptr<server> _server;

  // config
  std::string             _path;
  my::interval_t              _flush_interval;
  retention_policy        _default_policy;

  t_metrics_map           _metrics;
  spinlock                _metrics_lock;

  // TODO: make it a pool?
  boost::shared_ptr< boost::thread > _flush_to_disk_thread;
}; // class rrdb

#endif /* RRDB_H_ */
