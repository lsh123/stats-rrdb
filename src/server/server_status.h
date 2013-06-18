/*
 * server_status.h
 *
 *  Created on: Jun 17, 2013
 *      Author: aleksey
 */

#ifndef SERVER_STATUS_H_
#define SERVER_STATUS_H_

class server_status
{
public:
  server_status();
  virtual ~server_status();

  bool is_valid() const;
}; //  class server_status

#endif /* SERVER_STATUS_H_ */
