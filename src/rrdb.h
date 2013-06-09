/*
 * rrdb.h
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#ifndef RRDB_H_
#define RRDB_H_

#include <string>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "spinlock.h"
#include "parser/interval.h"
#include "parser/retention_policy.h"

// forward
class config;
class rrdb_metric;

class rrdb :
    public boost::enable_shared_from_this<rrdb>
{
public:
  typedef std::vector<boost::asio::const_buffer> t_result_buffers;
  typedef boost::unordered_map< std::string, boost::shared_ptr<rrdb_metric> > t_metrics_map;

public:
  rrdb(boost::shared_ptr<config> config);
  virtual ~rrdb();

  void start();

  // metrics operations
  boost::shared_ptr<rrdb_metric> create_metric(const std::string & name, const retention_policy & policy);
  void drop_metric(const std::string & name);
  boost::shared_ptr<rrdb_metric> find_metric(const std::string & name);
  boost::shared_ptr<rrdb_metric> get_metric(const std::string & name);

  // commands
  t_result_buffers execute_long_command(const std::vector<char> & buffer);
  void execute_short_command(const std::vector<char> & buffer);

private:


private:
  // config
  std::string      _path;
  interval_t       _flush_interval;
  retention_policy _default_policy;

  t_metrics_map    _metrics_map;
  spinlock         _metrics_map_lock;
}; // class rrdb

#endif /* RRDB_H_ */
