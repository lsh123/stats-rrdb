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


#include "exception.h"
#include "log.h"

statement statement_parse_tcp(std::string::const_iterator beg, std::string::const_iterator end)
{
  statement ret;
  statement_grammar<std::string::const_iterator> grammar;

  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Unable to parse the statement '%s'", std::string(beg, end).c_str());
  }

  return ret;
}

statement statement_parse_tcp(const std::string & str)
{
  return statement_parse_tcp(str.begin(), str.end());
}

statement statement_parse_tcp(std::vector<char>::const_iterator beg, std::vector<char>::const_iterator end)
{
  statement ret;
  statement_grammar<std::vector<char>::const_iterator> grammar;

  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Unable to parse the statement");
  }

  return ret;
}



statement statement_parse_udp(const std::string & str)
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
      std::size_t sz = data.size();
      if(sz != 3 && sz != 4) {
          throw exception("Invalid request: 'update' command expects 2 or 3 arguments - 'u|<name>|<value>[|<timestamp>]'");
      }

      statement_update st;
      st._name  = data[1];
      st._value = boost::lexical_cast<double>(data[2]);
      st._ts    = sz == 4 ? boost::lexical_cast<boost::int64_t>(data[3]) : boost::optional<boost::int64_t>();
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

