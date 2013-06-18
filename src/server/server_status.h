/*
 * server_status.h
 *
 *  Created on: Jun 17, 2013
 *      Author: aleksey
 */

#ifndef SERVER_STATUS_H_
#define SERVER_STATUS_H_

#include <string>

#include <boost/unordered_map.hpp>
#include <boost/variant.hpp>

class server_status
{
public:
  typedef boost::variant<
      bool,
      boost::int64_t,
      boost::uint64_t,
      double,
      std::string
  > t_value;
  typedef boost::unordered_map< std::string, t_value >         t_values_map;

public:
  server_status();
  virtual ~server_status();

  void add_value(const std::string & key, const t_value & value);

  bool is_valid() const;
  const time_t & get_create_ts() const;
  const t_values_map & get_values() const;


private:
  time_t        _max_valid_age;
  time_t        _create_ts;
  t_values_map  _values;
}; //  class server_status

#endif /* SERVER_STATUS_H_ */
