/*
 * exception.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#include "exception.h"

exception::exception(const std::string & msg)
{
  this->_msg = msg;
}

exception::~exception() throw ()
{

}

// exception
const char * exception::what() const throw()
{
  return this->_msg.c_str();
}

