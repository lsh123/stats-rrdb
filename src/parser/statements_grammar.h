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
#include "common/exception.h"

#include "statements.h"

#include "retention_policy.h"
#include "retention_policy_grammar.h"

BOOST_FUSION_ADAPT_STRUCT(
    statement_create,
    (std::string,      _name)
    (t_retention_policy, _policy)
)
BOOST_FUSION_ADAPT_STRUCT(
    statement_drop,
    (std::string,      _name)
)
BOOST_FUSION_ADAPT_STRUCT(
    statement_update,
    (std::string,      _name)
    (double,           _value)
    (boost::optional<my::time_t>, _ts)
)
BOOST_FUSION_ADAPT_STRUCT(
    statement_select,
    (std::string,      _name)
    (my::time_t,   _ts_begin)
    (my::time_t,   _ts_end)
    (boost::optional<my::interval_t>,     _group_by)
)
BOOST_FUSION_ADAPT_STRUCT(
    statement_show_policy,
    (std::string,      _name)
)
BOOST_FUSION_ADAPT_STRUCT(
    statement_show_metrics,
    (boost::optional<std::string>,      _like)
)
BOOST_FUSION_ADAPT_STRUCT(
    statement_show_status,
    (boost::optional<std::string>,      _like)
)


template<typename Iterator>
class metric_name_grammar:
    public qi::grammar < Iterator, std::string(), ascii::space_type >
{
  typedef qi::grammar < Iterator, std::string(), ascii::space_type > base_type;

private:
  qi::rule < Iterator, std::string(), ascii::space_type >      _start;

public:
  metric_name_grammar(bool enable_upper_case):
    base_type(_start)
  {
    if(enable_upper_case) {
        _start %= +qi::char_("a-zA-Z0-9._-");
    } else {
        _start %= +qi::char_("a-z0-9._-");
    }
  }
}; // metric_name_grammar


template<typename Iterator>
class statement_grammar:
    public qi::grammar < Iterator, t_statement(), ascii::space_type >
{
  typedef qi::grammar < Iterator, t_statement(), ascii::space_type > base_type;

private:
  qi::rule < Iterator, t_statement(), ascii::space_type >      _start;

  metric_name_grammar< Iterator >                              _name;
  qi::rule < Iterator, std::string(), ascii::space_type >      _quoted_name;
  t_retention_policy_grammar<Iterator>                         _policy;
  interval_grammar< Iterator >                                 _interval;

  qi::rule < Iterator, t_statement(),            ascii::space_type > _statement;
  qi::rule < Iterator, statement_create(),       ascii::space_type > _statement_create;
  qi::rule < Iterator, statement_drop(),         ascii::space_type > _statement_drop;
  qi::rule < Iterator, statement_update(),       ascii::space_type > _statement_update;
  qi::rule < Iterator, statement_select(),       ascii::space_type > _statement_select;
  qi::rule < Iterator, statement_show_policy(),  ascii::space_type > _statement_show_policy;
  qi::rule < Iterator, statement_show_metrics(), ascii::space_type > _statement_show_metrics;
  qi::rule < Iterator, statement_show_status(),  ascii::space_type > _statement_show_status;

public:
  statement_grammar():
    base_type(_start),
    _name(true) // enable upper case in metric names
  {
    // there is a bug in boost with handling single member structures:
    //
    // http://stackoverflow.com/questions/7770791/spirit-unable-to-assign-attribute-to-single-element-struct-or-fusion-sequence
    //

     _quoted_name %=
         ('"' > _name > '"') | ("'" > _name > "'")
     ;

     _statement_update %=
        nocaselit("update") > -nocaselit("metric")
        > _quoted_name
        > nocaselit("add") > qi::double_
        > nocaselit("at") > qi::ulong_
     ;

     _statement_select %=
        nocaselit("select") > nocaselit("*")
        > nocaselit("from") > -nocaselit("metric")
        > _quoted_name
        > nocaselit("between") > qi::ulong_ > nocaselit("and") > qi::ulong_
        > -(nocaselit("group") > nocaselit("by") > _interval)
     ;

     _statement_create %=
        nocaselit("create") > -nocaselit("metric")
        > _quoted_name
        > nocaselit("keep") > _policy
     ;

    _statement_drop %=
        nocaselit("drop") > -nocaselit("metric")
        > _quoted_name
        >> boost::spirit::eps
    ;

    _statement_show_policy %=
        nocaselit("show") >> -nocaselit("metric") >> nocaselit("policy")
        > _quoted_name
        >> boost::spirit::eps
    ;

    _statement_show_metrics %=
        nocaselit("show") >> nocaselit("metrics")
        > -(nocaselit("like") > _quoted_name)
        >> boost::spirit::eps
    ;

    _statement_show_status %=
        nocaselit("show") >> nocaselit("status")
        > -(nocaselit("like") > _quoted_name)
        >> boost::spirit::eps
    ;

    //
    // DONE!
    //
    _statement %=
        (
            _statement_create |
            _statement_drop   |
            _statement_update |
            _statement_select |
            _statement_create |
            _statement_show_policy   |
            _statement_show_metrics  |
            _statement_show_status
        )
        >> nocaselit(";")
    ;
    _start %= _statement;

    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler(qi::_1, qi::_2, qi::_3, qi::_4)
    );

  }
}; // statement_grammar


#endif /* STATEMENTS_GRAMMAR_H_ */
