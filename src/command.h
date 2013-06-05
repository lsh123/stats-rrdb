/*
 * command.h
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include "thread_pool.h"

class command: public thread_pool_task
{
public:
  command();
  virtual ~command();

public:
  // thread_pool_task
  void run();

public:
  virtual void send_response(const std::string & message) = 0;
};

#endif /* COMMAND_H_ */
