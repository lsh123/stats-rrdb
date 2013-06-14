/*
 * statements.cpp
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#include <sstream>

#include <boost/algorithm/string.hpp>

#include "statements.h"
#include "statements_grammar.h"


#include "exception.h"
#include "log.h"

statement statement_parse(std::string::const_iterator beg, std::string::const_iterator end)
{
  statement ret;
  statement_grammar<std::string::const_iterator> grammar;

  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Unable to parse the statement '%s'", std::string(beg, end).c_str());
  }

  return ret;
}

statement statement_parse(const std::string & str)
{
  return statement_parse(str.begin(), str.end());
}

statement statement_parse(std::vector<char>::const_iterator beg, std::vector<char>::const_iterator end)
{
  statement ret;
  statement_grammar<std::vector<char>::const_iterator> grammar;

  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Unable to parse the statement");
  }

  return ret;
}


statement statement_parse_short(const std::string & str)
{
  std::vector< std::string > data;
  boost::algorithm::split(data, str, boost::algorithm::is_any_of("|"), boost::algorithm::token_compress_off);

  return statement();
}
