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

#include "interval.h"
#include "retention_policy.h"

/**
 * CREATE statement string representation:
 *
 * CREATE METRIC "<name>" KEEP <policy> ;
 *
 */
typedef struct statement_create_
{
  std::string      _name;
  retention_policy _policy;
} statement_create;

/**
 * DROP statement string representation:
 *
 * DROP METRIC "<name>" ;
 *
 */
typedef struct statement_drop_
{
  std::string      _name;
} statement_drop;


/**
 * UPDATE statement string representation:
 *
 * UPDATE [METRIC] "<name>" ADD <value> AT <timestamp>;
 *
 */
typedef struct statement_update_
{
  std::string      _name;
  double           _value;
  boost::int64_t   _ts;
} statement_update;


/**
 * SELECT statement string representation:
 *
 * SELECT * FROM [METRIC] "<name>" BETWEEN <timestamp1> AND <timestamp2> ;
 *
 */
typedef struct statement_select_
{
  std::string      _name;
  boost::int64_t   _ts_begin;
  boost::int64_t   _ts_end;
} statement_select;


/**
 * SHOW METRIC POLICY statement string representation:
 *
 * SHOW METRIC POLICY "<name>" ;
 *
 */
typedef struct statement_show_policy_
{
  std::string      _name;
} statement_show_policy;

/**
 * SHOW METRICS print all the metrics available
 *
 * SHOW METRICS LIKE "<name>";
 *
 */
typedef struct statement_show_metrics_
{
  std::string      _like;
} statement_show_metrics;

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
    statement_show_metrics
> statement;

statement statement_parse(const std::string & str);
statement statement_parse(std::string::const_iterator beg, std::string::const_iterator end);
statement statement_parse(std::vector<char>::const_iterator beg, std::vector<char>::const_iterator end);

#endif /* STATEMENTS_H_ */
