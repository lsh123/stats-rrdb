/*
 * memory_size.cpp
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */

#include "memory_size.h"
#include "memory_size_grammar.h"

#include "common/exception.h"
#include "common/log.h"


my::memory_size_t memory_size_parse(const std::string & str)
{
  my::memory_size_t ret;
  memory_size_grammar<std::string::const_iterator> grammar;

  std::string::const_iterator beg = str.begin();
  std::string::const_iterator end = str.end();
  phrase_parse(beg, end, grammar, ascii::space, ret);
  if (beg != end) {
      throw exception("Parser error: 'memory size' unexpected '%s'", std::string(beg, end).c_str());
  }

  return ret;
}

std::string memory_size_write(const my::memory_size_t & memory_size) {
  std::ostringstream ret;
  my::memory_size_t v;

  if(memory_size % MEMORY_SIZE_GIGABYTE == 0) {
      v = memory_size/MEMORY_SIZE_GIGABYTE;
      ret << v << memory_size_get_name(MEMORY_SIZE_GIGABYTE, v);
  } else if(memory_size % MEMORY_SIZE_MEGABYTE == 0) {
      v = memory_size/MEMORY_SIZE_MEGABYTE;
      ret << v << memory_size_get_name(MEMORY_SIZE_MEGABYTE, v);
  } else if(memory_size % MEMORY_SIZE_KILOBYTE == 0) {
      v = memory_size/MEMORY_SIZE_KILOBYTE;
      ret << v << memory_size_get_name(MEMORY_SIZE_KILOBYTE, v);
  } else {
      v = memory_size;
      ret << v << " " << memory_size_get_name(MEMORY_SIZE_BYTE, v);
  }

  return ret.str();
}

std::string memory_size_get_name(const my::memory_size_t & unit, const my::memory_size_t & val)
{
  switch(unit) {
  case MEMORY_SIZE_BYTE:
    return (val > 1) ? "bytes" : "byte";
  case MEMORY_SIZE_KILOBYTE:
    return (val > 1) ? "KB" : "KB";
  case MEMORY_SIZE_MEGABYTE:
    return (val > 1) ? "MB" : "MB";
  case MEMORY_SIZE_GIGABYTE:
    return (val > 1) ? "GB" : "GB";
  default:
    throw exception("Unknown memory size unit %u", unit);
  }
}


