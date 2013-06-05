/*
 * log.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef LOG_H_
#define LOG_H_

#include <string>
#include <stdarg.h>

#include <boost/shared_ptr.hpp>

class config;

class log
{
public:
  enum levels {
    LEVEL_CRITICAL = 0,
    LEVEL_ERROR    = 1,
    LEVEL_WARNING  = 2,
    LEVEL_INFO     = 3,
    LEVEL_DEBUG    = 4
  };

public:
  static void init(const config & config);

  // MAIN LOGGING FUNCTION
  inline static void write(enum levels level, const char * msg, ...) {
    if(log::_log_level >= level) {
      va_list args;
      va_start(args, msg);
      log::write(level, msg, args);
      va_end (args);
    }
  }

private:
  static void write(enum levels level, const char * msg, va_list args);
  static std::string level2str(enum levels level);
  static enum levels str2level(const std::string & str);
  static std::string format_time();

private:
  static enum levels _log_level;
  static std::string _log_dest;
  static std::string _log_time_format;
};

#endif /* LOG_H_ */
