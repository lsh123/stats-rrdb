/*
 * server_status.cpp
 *
 *  Created on: Jun 17, 2013
 *      Author: aleksey
 */

#include "server_status.h"

server_status::server_status() :
  _max_valid_age(1), // 1 sec, configure later
  _create_ts(time(NULL))
{
  // TODO Auto-generated constructor stub

}

server_status::~server_status()
{
  // TODO Auto-generated destructor stub
}

void server_status::add_value(const std::string & key, const server_status::t_value & value)
{
  _values[key] = value;
}

bool server_status::is_valid() const
{
  return _create_ts + _max_valid_age < time(NULL);
}

const time_t & server_status::get_create_ts() const
{
  return _create_ts;
}

const server_status::t_values_map & server_status::get_values() const
{
  return _values;
}
