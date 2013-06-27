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
    base_type(_start, "metric name")
  {
    // rule definitions
    if(enable_upper_case) {
        _start %= +qi::char_("a-zA-Z0-9._-");
    } else {
        _start %= +qi::char_("a-z0-9._-");
    }

    // rule names
    _start.name("metric name");

    // errors
    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler(qi::_1, qi::_2, qi::_3, qi::_4)
    );
  }
}; // metric_name_grammar

template<typename Iterator>
class metric_quoted_name_grammar:
    public qi::grammar < Iterator, std::string(), ascii::space_type >
{
  typedef qi::grammar < Iterator, std::string(), ascii::space_type > base_type;

private:
  metric_name_grammar< Iterator >                              _name;
  qi::rule < Iterator, std::string(), ascii::space_type >      _start;

public:
  metric_quoted_name_grammar(bool enable_upper_case):
    base_type(_start, "quoted metric name"),
    _name(enable_upper_case)
  {
    // rule definitions
    _start %=
        ('"' > _name > '"') | ("'" > _name > "'")
    ;

    // rule names
    _start.name("quoted metric name");

    // errors
    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler(qi::_1, qi::_2, qi::_3, qi::_4)
    );
  }
}; // metric_quoted_name_grammar


template<typename Iterator>
class statement_grammar:
    public qi::grammar < Iterator, t_statement(), ascii::space_type >
{
  typedef qi::grammar < Iterator, t_statement(), ascii::space_type > base_type;

private:
  metric_quoted_name_grammar< Iterator >                       _quoted_name;
  metric_quoted_name_grammar< Iterator >                       _quoted_like;
  t_retention_policy_grammar<Iterator>                         _policy;
  interval_grammar< Iterator >                                 _interval;

  qi::rule < Iterator, statement_create(),       ascii::space_type > _statement_create;
  qi::rule < Iterator, statement_drop(),         ascii::space_type > _statement_drop;
  qi::rule < Iterator, statement_update(),       ascii::space_type > _statement_update;
  qi::rule < Iterator, statement_select(),       ascii::space_type > _statement_select;
  qi::rule < Iterator, statement_show_policy(),  ascii::space_type > _statement_show_policy;
  qi::rule < Iterator, statement_show_metrics(), ascii::space_type > _statement_show_metrics;
  qi::rule < Iterator, statement_show_status(),  ascii::space_type > _statement_show_status;
  qi::rule < Iterator, t_statement(),            ascii::space_type > _statement;

  qi::rule < Iterator, t_statement(), ascii::space_type >      _start;

public:
  statement_grammar():
    base_type(_start),
    _quoted_name(true), // enable upper case in metric names
    _quoted_like(true)  // enable upper case in metric names
  {
    // there is a bug in boost with handling single member structures:
    // http://stackoverflow.com/questions/7770791/spirit-unable-to-assign-attribute-to-single-element-struct-or-fusion-sequence

    // rule definitions
    _statement_update %=
        nocaselit("update") > -nocaselit("metric")
        > _quoted_name
        > nocaselit("add") > qi::double_
        > -(nocaselit("at") > qi::ulong_)
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
        (
            nocaselit("drop") > -nocaselit("metric")
            > _quoted_name
        )
        >> boost::spirit::eps
    ;

    _statement_show_policy %=
        (
            nocaselit("show") >> -nocaselit("metric") >> nocaselit("policy")
            > _quoted_name
        )
        >> boost::spirit::eps
    ;

    _statement_show_metrics %=
        (
            nocaselit("show") >> nocaselit("metrics")
            > -(nocaselit("like") > _quoted_name)
        )
        >> boost::spirit::eps
    ;

    _statement_show_status %=
        (
            nocaselit("show") >> nocaselit("status")
            > -(nocaselit("like") > _quoted_name)
        )
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
        > nocaselit(";")
    ;
    _start %= _statement;

    // rule names

   _quoted_name.name("quoted metric name");
   _quoted_like.name("quoted metric name pattern");
   _policy.name("retention policy");
   _interval.name("interval");

   _statement_create.name("'create metric' statement");
   _statement_drop.name("'drop metric' statement");
   _statement_update.name("'update metric' statement");
   _statement_select.name("'select' statement");
   _statement_show_policy.name("'show policy' statement");
   _statement_show_metrics.name("'show metrics' statement");
   _statement_show_status.name("'show status' statement");
   _statement.name("statement");
   _start.name("statement");

    // errors
    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler(qi::_1, qi::_2, qi::_3, qi::_4)
    );

  }
}; // statement_grammar


#endif /* STATEMENTS_GRAMMAR_H_ */
