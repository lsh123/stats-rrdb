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

#include <boost/fusion/include/adapt_struct.hpp>

#include "common/exception.h"

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using phoenix::at_c;
using phoenix::push_back;

//
// No case
//
typedef boost::proto::result_of::deep_copy<
    BOOST_TYPEOF(ascii::no_case[qi::lit(std::string())])
>::type nocaselit_return_type;
inline nocaselit_return_type nocaselit(const std::string& keyword)
{
    return boost::proto::deep_copy(ascii::no_case[qi::lit(keyword)]);
}

//
// Error handler
//
struct grammar_error_handler_impl
{
   template <class, class, class, class>
   struct result { typedef void type; };
 
   template <class B, class E, class W, class I>
   void
   operator ()(B beg, E end, W where, I const & info) const
   {
     std::ostringstream res;
     res << "Parser error: expecting "
         << info
         << " at \""
         << std::string(where, end)
         << "\""
     ;
     throw exception(res.str());
   }
};

extern phoenix::function<grammar_error_handler_impl> grammar_error_handler;

#endif /* INTERVAL_GRAMMAR_H_ */
