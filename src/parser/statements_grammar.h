/*
 * statements_grammar.h
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#ifndef STATEMENTS_GRAMMAR_H_
#define STATEMENTS_GRAMMAR_H_

#include "grammar.h"
#include "interval.h"
#include "exception.h"

#include "statements.h"

#include "retention_policy.h"
#include "retention_policy_grammar.h"

BOOST_FUSION_ADAPT_STRUCT(
    statement_create,
    (std::string,      _name)
    (retention_policy, _policy)
)
BOOST_FUSION_ADAPT_STRUCT(
    statement_drop,
    (std::string,      _name)
)

template<typename Iterator>
class statement_grammar:
    public qi::grammar < Iterator, statement(), ascii::space_type >
{
  typedef qi::grammar < Iterator, statement(), ascii::space_type > base_type;

private:
  qi::rule < Iterator, statement(), ascii::space_type >        _start;

  qi::rule < Iterator, std::string(), ascii::space_type >      _name;
  retention_policy_grammar<Iterator>                           _policy;

  qi::rule < Iterator, statement(),        ascii::space_type > _statement;
  qi::rule < Iterator, statement_create(), ascii::space_type > _statement_create;
  qi::rule < Iterator, statement_drop(),   ascii::space_type > _statement_drop;

public:
  statement_grammar():
    base_type(_start)
  {
    _name %=
       qi::lexeme['"' >> +(ascii::char_ - '"') >> '"'] |
       qi::lexeme["'" >> +(ascii::char_ - "'") >> "'"]
    ;
    _statement_create %=
        nocaselit("create") >> nocaselit("metric") >> _name >> nocaselit("KEEP") >> _policy
//        [ qi::_val = phoenix::construct<statement_create>() ]
     ;
    _statement_drop =
        nocaselit("drop") >> nocaselit("metric") >> _name
        [ qi::_val = phoenix::construct<statement_drop>() ]
    ;
    _statement %=
        (
            _statement_create |
            _statement_drop
        )
        >> nocaselit(";")
    ;
    _start %= _statement;

    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler("Error parsing statement", qi::_1, qi::_2, qi::_3, qi::_4)
    );
  }
}; // statement_grammar

#endif /* STATEMENTS_GRAMMAR_H_ */
