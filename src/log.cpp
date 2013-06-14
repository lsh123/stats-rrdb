/*
 * log.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>

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
  case log::LEVEL_DEBUG2:
    return "Debug2";
  case log::LEVEL_DEBUG3:
    return "Debug3";
  default:
    throw exception("Unknown log level %d", level);
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
  } else if(str_lower.compare("debug2") == 0) {
    return log::LEVEL_DEBUG2;
  } else if(str_lower.compare("debug3") == 0) {
    return log::LEVEL_DEBUG3;
  } else {
      throw exception("Unknown log level '%s'", str.c_str());
  }
}

int log::level2syslog(enum levels level)
{
  switch(level) {
  case log::LEVEL_CRITICAL:
    return LOG_USER | LOG_CRIT;
  case log::LEVEL_ERROR:
    return LOG_USER | LOG_ERR;
  case log::LEVEL_WARNING:
    return LOG_USER | LOG_WARNING;
  case log:: LEVEL_INFO:
    return LOG_USER | LOG_INFO;
  case log::LEVEL_DEBUG:
  case log::LEVEL_DEBUG2:
  case log::LEVEL_DEBUG3:
    return LOG_USER | LOG_DEBUG;
  default:
    throw exception("Unknown log level %d", level);
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
  LOG(log::LEVEL_DEBUG, "Configuring log file");

  // log level
  std::string log_level = config.get<std::string>("log.level", log::level2str(log::_log_level));
  log::_log_level = log::str2level(log_level);

  // log dest
  log::_log_dest = config.get<std::string>("log.destination", log::_log_dest);

  // log time format
  log::_log_time_format = config.get<std::string>("log.time_format", log::_log_time_format);

  //
  LOG(log::LEVEL_INFO, "Configured log file");
}

void log::write(enum levels level, const char * msg, ...) {
  va_list args;
  va_start(args, msg);
  log::write(level, msg, args);
  va_end (args);
}

void log::write(enum levels level, const char * msg, va_list args)
{
  // just to be sure
  if(!log::check_level(level)) {
      return;
  }

  char buffer[4096];
  vsnprintf(buffer, sizeof(buffer), msg, args);
  try {
    if(log::_log_dest == "stderr") {
        std::cerr << log::format_time() << " - " << log::level2str(level) << " - " << buffer << std::endl;
    } else if(log::_log_dest == "syslog") {
        syslog(log::level2syslog(level), buffer);
    } else {
        std::fstream ofs(log::_log_dest.c_str(), std::ios_base::app | std::ios_base::out);
        ofs << log::format_time() << " - " << log::level2str(level) << " - " << buffer << std::endl;
        ofs.flush();
        ofs.close();
    }
  } catch(...) {
      std::cerr << log::format_time() << " - " << log::level2str(log::LEVEL_CRITICAL) << " - " << "Unable to write to log: " << log::_log_dest << std::endl;
      std::terminate();
  }
}
