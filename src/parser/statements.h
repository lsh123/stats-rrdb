/*
 * statements.h
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#ifndef STATEMENTS_H_
#define STATEMENTS_H_

#include <string>
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
 * SHOW statement string representation:
 *
 * SHOW METRIC "<name>" ;
 *
 */
typedef struct statement_show_
{
  std::string      _name;
} statement_show;

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
    statement_show,
    statement_show_metrics_
> statement;

statement statement_parse(const std::string & str);
statement statement_parse(std::string::const_iterator beg, std::string::const_iterator end);

#endif /* STATEMENTS_H_ */
