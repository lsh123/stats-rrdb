/*
 * log.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */
#include <iostream>
#include <stdio.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/algorithm/string.hpp>

#include "exception.h"
#include "log.h"
#include "config.h"

// Statics
enum log::levels log::_log_level  = log::LEVEL_INFO;
std::string log::_log_dest        = "stderr";
std::string log::_log_time_format = "%m/%d/%Y %H:%M:%S";


std::string log::level2str(enum log::levels level)
{
  switch(level) {
  case log::LEVEL_CRITICAL:
    return "Critical";
  case log::LEVEL_ERROR:
    return "Error";
  case log::LEVEL_WARNING:
    return "Warning";
  case log:: LEVEL_INFO:
    return "Info";
  case log::LEVEL_DEBUG:
    return "Debug";
  default:
    throw exception("Unknown log level");
  }
}

enum log::levels log::str2level(const std::string & str)
{
  std::string str_lower = boost::to_lower_copy(str);
  if(str_lower == "critical") {
      return log::LEVEL_CRITICAL;
  } else if(str_lower.compare("error") == 0) {
    return log::LEVEL_ERROR;
  } else if(str_lower.compare("warning") == 0) {
    return log::LEVEL_WARNING;
  } else if(str_lower.compare("info") == 0) {
    return log::LEVEL_INFO;
  } else if(str_lower.compare("debug") == 0) {
    return log::LEVEL_DEBUG;
  } else {
      throw exception("Unknown log level '%s'", str.c_str());
  }
}

std::string log::format_time()
{
  std::locale loc(std::cout.getloc(), new boost::posix_time::time_facet(log::_log_time_format.c_str()));
  std::stringstream ss;

  ss.imbue(loc);
  ss << boost::posix_time::second_clock::universal_time();
  return ss.str();
}

void log::init(const config & config)
{
  log::write(log::LEVEL_DEBUG, "Configuring log file");

  // log level
  std::string log_level = config.get<std::string>("log.level", log::level2str(log::_log_level));
  log::_log_level = log::str2level(log_level);

  // log dest
  log::_log_dest = config.get<std::string>("log.destination", log::_log_dest);

  // log time format
  log::_log_time_format = config.get<std::string>("log.time_format", log::_log_time_format);

  //
  log::write(log::LEVEL_INFO, "Configured log file");
}

void log::write(enum levels level, const char * msg, va_list args)
{
  char buffer[4096];
  vsnprintf(buffer, sizeof(buffer), msg, args);

  // TODO: different log dest?
  std::cerr << log::format_time() << " - " << log::level2str(level) << " - " << buffer << std::endl;
}
