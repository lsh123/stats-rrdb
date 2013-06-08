/*
 * interval.cpp
 *
 *  Created on: Jun 6, 2013
 *      Author: aleksey
 */

#include "interval.h"
#include "interval_grammar.h"

#include "exception.h"
#include "log.h"


boost::uint64_t interval_parse(std::string::const_iterator beg, std::string::const_iterator end)
{
  boost::uint64_t ret;
  interval_grammar<std::string::const_iterator> grammar;

  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Unable to parse the interval: " + std::string(beg, end));
  }

  return ret;
}

std::string interval_write(const interval_t & interval) {
  std::ostringstream ret;
  interval_t v;

  if(interval % INTERVAL_YEAR == 0) {
      v = interval/INTERVAL_YEAR;
      ret << v << " " << interval_get_name(INTERVAL_YEAR, v);
  } else if(interval % INTERVAL_MONTH == 0) {
      v = interval/INTERVAL_MONTH;
      ret << v << " " << interval_get_name(INTERVAL_MONTH, v);
  } else if(interval % INTERVAL_WEEK == 0) {
      v = interval/INTERVAL_WEEK;
      ret << v << " " << interval_get_name(INTERVAL_WEEK, v);
  } else if(interval % INTERVAL_DAY == 0) {
      v = interval/INTERVAL_DAY;
      ret << v << " " << interval_get_name(INTERVAL_DAY, v);
  } else if(interval % INTERVAL_HOUR == 0) {
      v = interval/INTERVAL_HOUR;
      ret << v << " " << interval_get_name(INTERVAL_HOUR, v);
  } else if(interval % INTERVAL_MIN == 0) {
      v = interval/INTERVAL_MIN;
      ret << v << " " << interval_get_name(INTERVAL_MIN, v);
  } else {
      v = interval;
      ret << v << " " << interval_get_name(INTERVAL_SEC, v);
  }

  return ret.str();
}

std::string interval_get_name(const interval_t & unit, const interval_t & val)
{
  switch(unit) {
  case INTERVAL_SEC:
    return (val > 1) ? "secs" : "sec";
  case INTERVAL_MIN:
    return (val > 1) ? "mins" : "min";
  case INTERVAL_HOUR:
    return (val > 1) ? "hours" : "hour";
  case INTERVAL_DAY:
    return (val > 1) ? "days" : "day";
  case INTERVAL_WEEK:
    return (val > 1) ? "weeks" : "week";
  case INTERVAL_MONTH:
    return (val > 1) ? "months" : "month";
  case INTERVAL_YEAR:
    return (val > 1) ? "years" : "year";
  default:
    throw exception("Unknown interval unit");
  }
}


