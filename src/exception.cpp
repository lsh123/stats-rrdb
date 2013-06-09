/*
 * exception.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#include <stdarg.h>
#include <stdio.h>

#include "exception.h"

exception::exception(const std::string & msg) :
    _msg(msg)
{
}

exception::exception(const char * msg, ...)
{
  char buffer[4096];

  // setup the error message
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, sizeof(buffer), msg, args);
  va_end (args);

  _msg = buffer;
}

exception::~exception() throw ()
{

}

// exception
const char * exception::what() const throw()
{
  return _msg.c_str();
}

