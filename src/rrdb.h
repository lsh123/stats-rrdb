/*
 * rrdb.h
 *
 *  Created on: Jun 3, 2013
 *      Author: aleksey
 */

#ifndef RRDB_H_
#define RRDB_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

class rrdb_series {

  std::string _name;
}; // rrdb_series

class rrdb
{
public:
  rrdb();
  virtual ~rrdb();

  boost::shared_ptr<rrdb_series> getMetric(const std::string & name);
  boost::shared_ptr<rrdb_series> addMetric(const std::string & name, const std::string & definition);
  void delMetric(const std::string & name);

  void load(const std::string & filename);
  void save(const std::string & filename);

private:
  boost::unordered_map<std::string, boost::shared_ptr<rrdb_series> > _series;
}; // class rrdb

#endif /* RRDB_H_ */
