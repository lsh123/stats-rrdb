/*
 * statements.cpp
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "statements.h"
#include "statements_grammar.h"


#include "common/exception.h"
#include "common/log.h"

// TODO: better error handled
t_statement statement_query_parse(const std::string & str)
{
  t_statement ret;
  statement_grammar<std::string::const_iterator> grammar;
  std::string::const_iterator beg(str.begin());
  std::string::const_iterator end(str.end());
  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Unable to parse the statement '%s'", std::string(beg, end).c_str());
  }

  return ret;
}

t_statement statement_update_parse(const std::string & str)
{
  std::vector< std::string > data;
  boost::algorithm::split(data, str, boost::algorithm::is_any_of("|"), boost::algorithm::token_compress_off);
  if(data.empty()) {
      throw exception("Invalid request: command is missing");
  }

  const std::string & cmd(data[0]);
  if(cmd == "u" || cmd == "U") {
      // Update metric
      //
      // u|<name>|<value>|<timestamp>
      my::size_t sz = data.size();
      if(sz != 3 && sz != 4) {
          throw exception("Invalid request: 'update' command expects 2 or 3 arguments - 'u|<name>|<value>[|<timestamp>]'");
      }

      statement_update st;
      st._name  = data[1];
      st._value = boost::lexical_cast<double>(data[2]);
      st._ts    = sz == 4 ? boost::lexical_cast<my::time_t>(data[3]) : boost::optional<my::time_t>();
      return st;
  } else if(cmd == "c" || cmd == "C") {
      // Create metric
      //
      // c|<name>|<retention policy>
      if(data.size() != 3) {
          throw exception("Invalid request: 'create' command expects 2 arguments - 'c|<name>|<retention policy>'");
      }

      statement_create st;
      st._name   = data[1];
      st._policy = retention_policy_parse(data[2]);
      return st;
  } else if(cmd == "d" || cmd == "D") {
      // Drop metric
      //
      // d|<name>
      if(data.size() != 2) {
          throw exception("Invalid request: 'drop' command expects 1 argument - 'd|<name>'");
      }

      statement_drop st;
      st._name   = data[1];
      return st;
  }

  // no luck
  throw exception("Invalid request: command '%s' is unknow", cmd.c_str());
}

