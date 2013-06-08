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

#include "parser/interval.h"
#include "parser/retention_policy.h"

// forward
class config;

class rrdb :
    public boost::enable_shared_from_this<rrdb>
{
public:
  typedef std::vector<boost::asio::const_buffer> t_result_buffers;

public:
  rrdb(boost::shared_ptr<config> config);
  virtual ~rrdb();

  void start();

  void create_metric(const std::string & name, const retention_policy & policy);
  void drop_metric(const std::string & name);
  retention_policy get_metric_policy(const std::string & name);

  t_result_buffers execute_long_command(const std::vector<char> & buffer);
  void execute_short_command(const std::vector<char> & buffer);

private:
  // config
  std::string      _path;
  interval_t       _flush_interval;
  retention_policy _default_policy;
}; // class rrdb

#endif /* RRDB_H_ */
