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
#include "exception.h"

#include "retention_policy.h"
#include "interval_grammar.h"

BOOST_FUSION_ADAPT_STRUCT(
    retention_policy_elem,
    (my::interval_t, _freq)
    (my::interval_t, _duration)
)

template<typename Iterator>
class retention_policy_grammar:
    public qi::grammar < Iterator, retention_policy(), ascii::space_type >
{
  typedef qi::grammar < Iterator, retention_policy(), ascii::space_type > base_type;

private:
  qi::rule < Iterator, retention_policy(), ascii::space_type >      _start;
  qi::rule < Iterator, retention_policy_elem(), ascii::space_type > _elem;
  interval_grammar< Iterator > _interval;

public:
  retention_policy_grammar():
    base_type(_start)
  {
    _elem  %= _interval >> nocaselit("for") >> _interval;
    _start %= _elem  >> *( nocaselit(",") >> _elem );

    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler("Error parsing retention policy", qi::_1, qi::_2, qi::_3, qi::_4)
    );
  }
}; // retention_policy_grammar

#endif /* RETENTION_POLICY_GRAMMAR_H_ */
