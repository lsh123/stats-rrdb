/*
 * interval_grammar.h
 *
 *  Created on: Jun 6, 2013
 *      Author: aleksey
 */

#ifndef INTERVAL_GRAMMAR_H_
#define INTERVAL_GRAMMAR_H_

#include "grammar.h"
#include "interval.h"
#include "common/exception.h"

#include "interval.h"

template<typename Iterator>
class interval_grammar:
    public qi::grammar < Iterator, my::interval_t(), ascii::space_type >
{
  typedef qi::grammar < Iterator, my::interval_t(), ascii::space_type > base_type;

private:
  qi::rule < Iterator, my::interval_t(), ascii::space_type >  _value;
  qi::rule < Iterator, my::interval_t(), ascii::space_type >  _unit;
  qi::rule < Iterator, my::interval_t(), ascii::space_type >  _start;

public:
  interval_grammar():
    base_type(_start, "interval")
  {
    // rule definitions
    _value %=
        qi::uint_
    ;
    _unit  = (
        nocaselit("secs")    [ qi::_val = INTERVAL_SEC ]
      | nocaselit("sec")     [ qi::_val = INTERVAL_SEC ]
      | nocaselit("mins")    [ qi::_val = INTERVAL_MIN ]
      | nocaselit("min")     [ qi::_val = INTERVAL_MIN ]
      | nocaselit("hours")   [ qi::_val = INTERVAL_HOUR ]
      | nocaselit("hour")    [ qi::_val = INTERVAL_HOUR ]
      | nocaselit("days")    [ qi::_val = INTERVAL_DAY ]
      | nocaselit("day")     [ qi::_val = INTERVAL_DAY ]
      | nocaselit("weeks")   [ qi::_val = INTERVAL_WEEK ]
      | nocaselit("week")    [ qi::_val = INTERVAL_WEEK ]
      | nocaselit("months")  [ qi::_val = INTERVAL_MONTH ]
      | nocaselit("month")   [ qi::_val = INTERVAL_MONTH ]
      | nocaselit("years")   [ qi::_val = INTERVAL_YEAR ]
      | nocaselit("year")    [ qi::_val = INTERVAL_YEAR ]
     );
    _start =
        _value               [ qi::_val = qi::_1 ]
      > _unit                [ qi::_val *= qi::_1 ]
     ;

    // rule names
    _start.name("interval");
    _value.name("interval value");
    _unit.name("interval unit");

    // errors
    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler(qi::_1, qi::_2, qi::_3, qi::_4)
    );
  }
}; // interval_grammar

#endif /* INTERVAL_GRAMMAR_H_ */
