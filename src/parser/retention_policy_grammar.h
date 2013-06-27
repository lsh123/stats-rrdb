/*
 * retention_policy_grammar.h
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#ifndef RETENTION_POLICY_GRAMMAR_H_
#define RETENTION_POLICY_GRAMMAR_H_

#include "grammar.h"
#include "interval.h"
#include "common/exception.h"

#include "retention_policy.h"
#include "interval_grammar.h"

BOOST_FUSION_ADAPT_STRUCT(
    t_retention_policy_elem,
    (my::interval_t, _freq)
    (my::interval_t, _duration)
)

template<typename Iterator>
class t_retention_policy_grammar:
    public qi::grammar < Iterator, t_retention_policy(), ascii::space_type >
{
  typedef qi::grammar < Iterator, t_retention_policy(), ascii::space_type > base_type;

private:
  qi::rule < Iterator, t_retention_policy(), ascii::space_type >      _start;
  qi::rule < Iterator, t_retention_policy_elem(), ascii::space_type > _elem;
  interval_grammar< Iterator > _freq;
  interval_grammar< Iterator > _duration;

public:
  t_retention_policy_grammar():
    base_type(_start, "retention policy")
  {
    // rule definitions
    _elem  %= _freq > nocaselit("for") >  _duration;
    _start %= qi::eps > _elem >> *(nocaselit(",") > _elem );

    // rule names
    _start.name("retention policy");
    _elem.name("retention policy element");
    _freq.name("retention policy element frequency");
    _duration.name("retention policy element duration");

    // errors
    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler(qi::_1, qi::_2, qi::_3, qi::_4)
    );
  }
}; // t_retention_policy_grammar

#endif /* RETENTION_POLICY_GRAMMAR_H_ */
