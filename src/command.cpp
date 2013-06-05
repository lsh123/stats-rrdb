/*
 * command.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: aleksey
 */

#include "command.h"

command::command()
{
  // TODO Auto-generated constructor stub

}

command::~command()
{
  // TODO Auto-generated destructor stub
}

// thread_pool_task
void command::run()
{
  this->send_response("OK");
}
