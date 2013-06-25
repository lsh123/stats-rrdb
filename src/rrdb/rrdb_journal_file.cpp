/*
 * rrdb_journal_file.cpp
 *
 *  Created on: Jun 25, 2013
 *      Author: aleksey
 */
#include <boost/foreach.hpp>

#include "parser/memory_size.h"

#include "rrdb/rrdb_journal_file.h"
#include "rrdb/rrdb_files_cache.h"

#include "common/config.h"
#include "common/log.h"
#include "common/exception.h"

rrdb_journal_file::rrdb_journal_file(const boost::shared_ptr<rrdb_files_cache> & files_cache) :
  _files_cache(files_cache),
  _cur_data(),
  _cur_data_stream(_cur_data)
{
}

rrdb_journal_file::~rrdb_journal_file()
{
}

// clean state
void rrdb_journal_file::reset()
{
  _cur_data_stream.clear();

  _cur_full_path.resize(0);
  _cur_data.resize(0);
  _cur_data_blocks.resize(0);
}

// starts/end writing data to the journal for a given file
std::ostream & rrdb_journal_file::begin_file(const std::string & full_path)
{
  CHECK_AND_THROW(!full_path.empty());
  CHECK_AND_THROW(_cur_full_path.empty());
  CHECK_AND_THROW(_cur_data.empty());
  CHECK_AND_THROW(_cur_data_blocks.empty());

  _cur_full_path = full_path;
  return _cur_data_stream;
}

void rrdb_journal_file::end_file()
{
  CHECK_AND_THROW(!_cur_full_path.empty());

  _cur_data_stream.flush(); // write all data into the buffer
}

// starts/ends writing data block to the currently specified file at a given offset
void rrdb_journal_file::add_block(const my::size_t & offset, const my::size_t & size)
{
  CHECK_AND_THROW(!_cur_full_path.empty());
  CHECK_AND_THROW(size > 0);

  // calculate the offset of the new block
  my::size_t journal_offset = 0;
  if(!_cur_data_blocks.empty()) {
      const t_data_block & prev_block(_cur_data_blocks.back());
      journal_offset = prev_block._journal_offset + prev_block._size;
  }

  // add new block
  t_data_block block;
  block._real_offset    = offset;
  block._journal_offset = journal_offset;
  block._size           = size;
  _cur_data_blocks.push_back(block);
}


void rrdb_journal_file::apply_journal(std::fstream & os)
{
  BOOST_FOREACH(const t_data_block & block, _cur_data_blocks) {
    os.seekg(block._real_offset, os.beg);
    os.write((const char*)&(_cur_data[block._journal_offset]), block._size);
  }
}

void rrdb_journal_file::apply_journal(const my::filename_t & filename)
{
  CHECK_AND_THROW(filename);

  // open real file and seek to the start of the file
  rrdb_files_cache::fstream_ptr fs(_files_cache->open_file(filename, time(NULL)));
  fs->sync();

  // apply the journal file to the real file
  this->apply_journal(*fs);

  // flush, don't close (this is the file from cache!)
  fs->flush();
  fs->sync();
}

// read/write internal journal buffer from/to the file
void rrdb_journal_file::load_journal_file()
{
}

void rrdb_journal_file::save_journal_file()
{
}

