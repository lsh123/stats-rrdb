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

#include "parser/interval.h"

// forward
class config;

class rrdb
{
public:
  typedef std::vector<boost::asio::const_buffer> t_result_buffers;

public:
  rrdb(boost::shared_ptr<config> config);
  virtual ~rrdb();

  t_result_buffers execute_long_command(const std::vector<char> & buffer);
  void execute_short_command(const std::vector<char> & buffer);

private:
  // config
  std::string    _path;
  interval_t     _flush_interval;
}; // class rrdb

#endif /* RRDB_H_ */
