/*
 * utils.h
 *
 *  Created on: Jun 25, 2013
 *      Author: aleksey
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>
#include <iostream>

#include "common/types.h"


// padded strings
class padded_string {
  typedef struct t_string_header_ {
    boost::uint16_t   _size;          // actual length of the name
    boost::uint16_t   _padded_size;   // 64-bit padded size of the name array
    boost::uint16_t   _unused1;
    boost::uint16_t   _unused2;
  } t_string_header;

public:
  padded_string(const std::string & str = std::string());
  void reset(const std::string & str);

  // I/O
  void read(std::istream & is);
  my::size_t write(std::ostream & os) const;

  // we guarantee it's \0 terminated
  inline const char * get() const
  {
    return &(_buf[0]);
  }
  inline my::size_t get_size() const
  {
    return _size;
  }
  inline my::size_t get_padded_size() const
  {
    return _buf.size();
  }
  inline my::size_t get_file_size() const
  {
    return sizeof(t_string_header) + this->get_padded_size();
  }

  inline bool empty() const
  {
    return _size == 0;
  }

  inline void clear()
  {
    _buf.clear();
    _size = 0;
  }

  inline padded_string & operator=(const std::string & str)
  {
    this->reset(str);
    return (*this);
  }



private:
  std::vector<char> _buf;
  my::size_t        _size;
}; // padded_string


#endif /* UTILS_H_ */
