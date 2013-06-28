/*
 * statements.h
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#ifndef STATEMENTS_H_
#define STATEMENTS_H_

#include <string>
#include <vector>

#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "interval.h"
#include "retention_policy.h"

/**
 * CREATE statement string representation:
 *
 * CREATE METRIC "<name>" KEEP <policy> ;
 *
 */
class statement_create
{
public:
  std::string      _name;
  boost::optional<t_retention_policy> _policy;
};

/**
 * DROP statement string representation:
 *
 * DROP METRIC "<name>" ;
 *
 */
class statement_drop
{
public:
  std::string      _name;
};


/**
 * UPDATE statement string representation:
 *
 * UPDATE [METRIC] "<name>" ADD <value> [ AT <timestamp> ] ;
 *
 */
class statement_update
{
public:
  std::string                 _name;
  double                      _value;
  boost::optional<my::time_t> _ts;
};


/**
 * SELECT statement string representation:
 *
 * SELECT * FROM [METRIC] "<name>" BETWEEN <timestamp1> AND <timestamp2> [ GROUP BY <interval> ];
 *
 */
class statement_select
{
public:
  std::string                     _name;
  my::time_t                      _ts_begin;
  my::time_t                      _ts_end;
  boost::optional<my::interval_t> _group_by;
};

/**
 * SHOW METRIC POLICY statement string representation:
 *
 * SHOW [ METRIC ] POLICY "<name>" ;
 *
 */
class statement_show_policy
{
public:
  std::string      _name;
};

/**
 * SHOW METRICS print all the metrics available
 *
 * SHOW METRICS [ LIKE "<name>" ] ;
 *
 */
class statement_show_metrics
{
public:
  boost::optional<std::string> _like;
};

/**
 * SHOW STATUS print status
 *
 * SHOW STATUS [ LIKE "<name>" ] ;
 *
 */
class statement_show_status
{
public:
  boost::optional<std::string> _like;
};

/**
 * Statement: any of the above
 *
 */
typedef boost::variant<
    statement_create,
    statement_drop,
    statement_update,
    statement_select,
    statement_show_policy,
    statement_show_metrics,
    statement_show_status
> t_statement;

// The above grammar
t_statement statement_query_parse(const std::string & str);

// <command>|<param1>|<param2>|....
t_statement statement_update_parse(const std::string & str);

// check the metric name
bool statement_check_metric_name(const std::string & str, bool allow_upper_case = false);


#endif /* STATEMENTS_H_ */
