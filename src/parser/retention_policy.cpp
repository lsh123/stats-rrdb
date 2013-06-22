/*
 * t_retention_policy.cpp
 *
 *  Created on: Jun 7, 2013
 *      Author: aleksey
 */

#include <sstream>

#include "retention_policy.h"
#include "retention_policy_grammar.h"


#include "common/exception.h"
#include "common/log.h"


t_retention_policy retention_policy_parse(const std::string & str)
{
  t_retention_policy ret;
  t_retention_policy_grammar<std::string::const_iterator> grammar;

  std::string::const_iterator beg = str.begin();
  std::string::const_iterator end = str.end();
  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Unable to parse retention policy '%s'", str.c_str());
  }

  retention_policy_validate(ret);

  return ret;
}

std::string retention_policy_write(const t_retention_policy & r_p)
{
  std::ostringstream ret;
  my::size_t count = 0;
  for(t_retention_policy::const_iterator it = r_p.begin(); it != r_p.end(); ++it, ++count) {
      if(count > 0) {
          ret << ", ";
      }
      ret << interval_write((*it)._freq) << " for " << interval_write((*it)._duration);
  }

  return ret.str();
}

void retention_policy_validate(const t_retention_policy & r_p)
{
  for(t_retention_policy::const_iterator it = r_p.begin(), it_prev = r_p.end(); it != r_p.end(); it_prev = (it++)) {
      if((*it)._duration % (*it)._freq != 0) {
          throw exception("Policy duration '%s' should be a multiplier for the frequency '%s'", interval_write((*it)._duration).c_str(), interval_write((*it)._freq).c_str());
      }
      if(it_prev != r_p.end() && (*it)._freq % (*it_prev)._freq != 0) {
          throw exception("Policy frequency '%s' should be a multiplier for the previous policy frequency '%s'", interval_write((*it)._freq).c_str(), interval_write((*it_prev)._freq).c_str());
      }
      if(it_prev != r_p.end() && (*it)._freq > (*it_prev)._duration) {
          throw exception("Policy frequency '%s' should be less or equal than the previous policy duration '%s'", interval_write((*it)._freq).c_str(), interval_write((*it_prev)._freq).c_str());
      }
  }
}

