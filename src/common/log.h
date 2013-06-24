/*
 * log.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_

#include <string>
#include <stdarg.h>

#include "common/types.h"
#include "common/exception.h"

class config;

#define SIZE_T_CAST (unsigned long)

class log
{
public:
  enum levels {
    LEVEL_CRITICAL = 0,
    LEVEL_ERROR    = 1,
    LEVEL_WARNING  = 2,
    LEVEL_INFO     = 3,
    LEVEL_DEBUG    = 4,
    LEVEL_DEBUG2   = 5,
    LEVEL_DEBUG3   = 6
   };

public:
  static void init(const config & config);

  // MAIN LOGGING FUNCTION
  inline static bool check_level(enum levels level)
  {
    return log::_log_level >= level;
  }

  static void write(enum levels level, const char * msg, ...);
  static void write(enum levels level, const char * msg, va_list args);

  static std::string get_log_file();

private:
  static std::string level2str(enum levels level);
  static enum levels str2level(const std::string & str);
  static std::string format_time();

  static int level2syslog(enum levels level);

private:
  static enum levels _log_level;
  static std::string _log_dest;
  static std::string _log_time_format;
}; // class log


#define LOG(level, ...)  \
    if(log::check_level(level)) { log::write(level, __VA_ARGS__); }

#endif /* COMMON_LOG_H_ */
