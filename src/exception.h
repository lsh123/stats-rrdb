/*
 * exception.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <string>

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
};

#endif /* EXCEPTION_H_ */
