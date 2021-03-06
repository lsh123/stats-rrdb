/*
 * exception.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef COMMON_EXCEPTION_H_
#define COMMON_EXCEPTION_H_

#include <string>

#include "common/log.h"
#include "common/types.h"

class exception: public std::exception
{
public:
  exception(const std::string & msg);
  exception(const char * msg, ...);
  virtual ~exception() throw () ;

public:
  // std::exception
  const char * what() const throw();

private:
  std::string _msg;
}; // class exception

//
// Helper asserts
//
#define CHECK_AND_THROW( p ) \
  if( !( p ) ) { \
      LOG(log::LEVEL_CRITICAL, "Assert failed: '%s' in file '%s' on line '%d' (function '%s')",  #p, __FILE__, __LINE__, __FUNCTION__); \
      throw exception("Unexpected error"); \
  }




#endif /* COMMON_EXCEPTION_H_ */

