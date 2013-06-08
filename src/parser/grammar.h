/*
 * grammar.h
 *
 *  Created on: Jun 6, 2013
 *      Author: aleksey
 */

#ifndef GRAMMAR_H_
#define GRAMMAR_H_

// Common grammar includes, namespaces, defines, etc
#include <sstream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "exception.h"

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using phoenix::at_c;
using phoenix::push_back;

struct grammar_error_handler_impl
{
   template <class, class, class, class, class>
   struct result { typedef void type; };
 
   template <class M, class B, class E, class W, class I>
   void
   operator ()(const M & msg, B beg, E end, W where, I const & info) const
   {
     std::ostringstream res;
     res
       << msg
       << info
       << " at \""
       << phoenix::construct<std::string>(where, end)
       << "\""
     ;
     throw exception(res.str());
   }
};
phoenix::function<grammar_error_handler_impl> grammar_error_handler;

#endif /* INTERVAL_GRAMMAR_H_ */
