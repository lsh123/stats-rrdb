/*
 * rrdb_metric_block.cpp
 *
 *  Created on: Jun 12, 2013
 *      Author: aleksey
 */

#include "rrdb_metric_block.h"


#include "log.h"
#include "exception.h"

#define RRDB_METRIC_BLOCK_MAGIC    0xBB99

rrdb_metric_block::rrdb_metric_block(boost::uint32_t freq, boost::uint32_t count, boost::uint64_t offset)
{
  memset(&_header, 0, sizeof(_header));
  _header._magic     = RRDB_METRIC_BLOCK_MAGIC;
  _header._freq      = freq;
  _header._count     = count;
  _header._offset    = offset;
  _header._data_size = _header._count * sizeof(rrdb_metric_tuple_t);

  if(_header._count > 0) {
      _tuples.reset(new rrdb_metric_tuple_t[_header._count]);
  }
}

rrdb_metric_block::~rrdb_metric_block()
{
}

void rrdb_metric_block::write_block(std::fstream & ofs)
{
  // write header
  ofs.write((const char*)&_header, sizeof(_header));

  // write data
  if(_tuples.get()) {
      ofs.write((const char*)_tuples.get(), _header._data_size);
  }
}

void rrdb_metric_block::read_block(std::fstream & ifs)
{
  // rememeber where are we
  std::streampos offset = ifs.tellg();

  // read header
  ifs.read((char*)&_header, sizeof(_header));
  if(_header._magic != RRDB_METRIC_BLOCK_MAGIC) {
      throw exception("Unexpected rrdb metric block magic: %04x", _header._magic);
  }
  if(_header._offset != offset) {
      throw exception("Unexpected rrdb metric block offset: %llu (expected %llu)", _header._offset, offset);
  }
  if(_header._data_size != _header._count * sizeof(rrdb_metric_tuple_t)) {
      throw exception("Unexpected rrdb metric block data size: %llu (expected %llu)", _header._data_size, _header._count * sizeof(rrdb_metric_tuple_t));
  }

  // read data
  if(_header._count > 0) {
      _tuples.reset(new rrdb_metric_tuple_t[_header._count]);
      ifs.read((char*)_tuples.get(), _header._data_size);
  } else {
      _tuples.reset();
  }
}
