/*
 * retention_policy.cpp
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#include <sstream>

#include "retention_policy.h"
#include "retention_policy_grammar.h"


#include "exception.h"
#include "log.h"


retention_policy retention_policy_parse(const std::string & str)
{
  retention_policy ret;
  retention_policy_grammar<std::string::const_iterator> grammar;

  std::string::const_iterator beg = str.begin();
  std::string::const_iterator end = str.end();
  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Unable to parse the retention policy: " + str);
  }

  retention_policy_validate(ret);

  return ret;
}

std::string retention_policy_write(const retention_policy & r_p)
{
  std::ostringstream ret;
  std::size_t count = 0;
  for(retention_policy::const_iterator it = r_p.begin(); it != r_p.end(); ++it, ++count) {
      if(count > 0) {
          ret << ", ";
      }
      ret << interval_write((*it)._freq) << " for " << interval_write((*it)._duration);
  }

  return ret.str();
}

void retention_policy_validate(const retention_policy & r_p)
{
  for(retention_policy::const_iterator it = r_p.begin(), it_prev = r_p.end(); it != r_p.end(); it_prev = (it++)) {
      if((*it)._duration % (*it)._freq != 0) {
          throw exception("Policy duration '" + interval_write((*it)._duration) + "' does not match the frequency '" + interval_write((*it)._freq) + "'");
      }
      if(it_prev != r_p.end() && (*it)._freq % (*it_prev)._freq != 0) {
          throw exception("Policy frequency '" + interval_write((*it)._freq) + "' does not match the previous policy frequency '" + interval_write((*it_prev)._freq) + "'");
      }
  }
}

