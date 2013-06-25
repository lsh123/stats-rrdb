/*
 * utils.cpp
 *
 *  Created on: Jun 25, 2013
 *      Author: aleksey
 */
#include <string.h>


#include "common/utils.h"
#include "common/log.h"
#include "common/exception.h"



#define PADDED_STRING_PAD_BYTES 8

padded_string::padded_string(const std::string & str) :
    _size(0)
{
  this->reset(str);
}

void padded_string::reset(const std::string & str)
{
  // set size
  _size = str.length();

  // add \0 at the end up to padded_size
  my::size_t padded_size = _size + (PADDED_STRING_PAD_BYTES - (_size % PADDED_STRING_PAD_BYTES)); // ALWAYS GREATER THAN _size
  _buf.resize(padded_size);
  std::fill(_buf.begin(), _buf.end(), 0);

  // copy
  std::copy(str.begin(), str.end(), _buf.begin());
}

// I/O
void padded_string::read(std::istream & is)
{
  // read header
  t_string_header header;
  memset(&header, 0, sizeof(header));
  is.read((char*)&header, sizeof(header));
  CHECK_AND_THROW(header._size < header._padded_size);

  // prepare buffer
  _size = header._size;
  _buf.resize(header._padded_size, 0);

  // read string
  is.read(&(_buf[0]), _buf.size());
}

my::size_t padded_string::write(std::ostream & os) const
{
  CHECK_AND_THROW(_size < _buf.size());

  my::size_t written_bytes(0);

  // write header
  t_string_header header;
  memset(&header, 0, sizeof(header));
  header._size        = _size;
  header._padded_size = _buf.size();
  os.write((const char*)&header, sizeof(header));
  written_bytes += sizeof(header);

  // write data
  os.write(&(_buf[0]), _buf.size());
  written_bytes += _buf.size();

  // done
  return written_bytes;
}
