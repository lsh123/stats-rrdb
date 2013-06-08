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
#include "exception.h"

template<typename Iterator>
class interval_grammar:
    public qi::grammar < Iterator, interval_t(), ascii::space_type >
{
  typedef qi::grammar < Iterator, interval_t(), ascii::space_type > base_type;

private:
  qi::rule < Iterator, interval_t(), ascii::space_type >  _start;

public:
  interval_grammar():
    base_type(_start)
  {
    _start =
          qi::ulong_            [ qi::_val = qi::_1 ]
          >>
          (
             qi::lit("secs")    [ qi::_val *= INTERVAL_SEC ]
           | qi::lit("sec")     [ qi::_val *= INTERVAL_SEC ]
           | qi::lit("mins")    [ qi::_val *= INTERVAL_MIN ]
           | qi::lit("min")     [ qi::_val *= INTERVAL_MIN ]
           | qi::lit("hours")   [ qi::_val *= INTERVAL_HOUR ]
           | qi::lit("hour")    [ qi::_val *= INTERVAL_HOUR ]
           | qi::lit("days")    [ qi::_val *= INTERVAL_DAY ]
           | qi::lit("day")     [ qi::_val *= INTERVAL_DAY ]
           | qi::lit("weeks")   [ qi::_val *= INTERVAL_WEEK ]
           | qi::lit("week")    [ qi::_val *= INTERVAL_WEEK ]
           | qi::lit("months")  [ qi::_val *= INTERVAL_MONTH ]
           | qi::lit("month")   [ qi::_val *= INTERVAL_MONTH ]
           | qi::lit("years")   [ qi::_val *= INTERVAL_YEAR ]
           | qi::lit("year")    [ qi::_val *= INTERVAL_YEAR ]
           | qi::eps            [ qi::_val *= INTERVAL_SEC ]
          )
     ;

    qi::on_error<qi::fail> (
        _start,
        grammar_error_handler("Error parsing interval", qi::_1, qi::_2, qi::_3, qi::_4)
    );
  }
}; // interval_grammar

#endif /* INTERVAL_GRAMMAR_H_ */
