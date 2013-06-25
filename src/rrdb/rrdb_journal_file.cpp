/*
 * rrdb_journal_file.cpp
 *
 *  Created on: Jun 25, 2013
 *      Author: aleksey
 */
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

#include "parser/memory_size.h"

#include "rrdb/rrdb_journal_file.h"
#include "rrdb/rrdb_files_cache.h"

#include "common/config.h"
#include "common/log.h"
#include "common/exception.h"

#define RRDB_JOURNAL_FILE_HEADER_MAGIC               0xDA99
#define RRDB_JOURNAL_BLOCK_HEADER_MAGIC              0xDA66

#define RRDB_JOURNAL_FILE_VERSION                    1

// journal file format:
// <header>              t_journal_file_header
// <file name>           padded_string
// <block1 header>       t_journal_block_header
// <block1 data>         data
// <block2 header>       t_journal_block_header
// <block2 data>         data
// ...
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
void rrdb_journal_file::clear()
{
  _cur_data_stream.clear();
  _cur_filename.clear();
  _cur_data.clear();
  _cur_data_blocks.clear();
}

// starts/end writing data to the journal for a given file
std::ostream & rrdb_journal_file::begin_file(const std::string & filename)
{
  CHECK_AND_THROW(!filename.empty());
  CHECK_AND_THROW(_cur_filename.empty());
  CHECK_AND_THROW(_cur_data.empty());
  CHECK_AND_THROW(_cur_data_blocks.empty());

  _cur_filename = filename;
  return _cur_data_stream;
}

void rrdb_journal_file::end_file()
{
  CHECK_AND_THROW(!_cur_filename.empty());

  _cur_data_stream.flush(); // write all data into the buffer
}

// starts/ends writing data block to the currently specified file at a given offset
void rrdb_journal_file::add_block(const my::size_t & offset, const my::size_t & size)
{
  CHECK_AND_THROW(!_cur_filename.empty());
  CHECK_AND_THROW(size > 0);

  // calculate the offset of the new block
  my::size_t journal_offset = 0;
  if(!_cur_data_blocks.empty()) {
      const t_journal_block_header & prev_block(_cur_data_blocks.back());
      journal_offset = prev_block._journal_offset + prev_block._size;
  }

  // add new block
  t_journal_block_header block;
  block._magic          = RRDB_JOURNAL_BLOCK_HEADER_MAGIC;
  block._real_offset    = offset;
  block._journal_offset = journal_offset;
  block._size           = size;
  _cur_data_blocks.push_back(block);
}


void rrdb_journal_file::apply_journal(std::fstream & os)
{
  BOOST_FOREACH(const t_journal_block_header & block, _cur_data_blocks) {
    os.seekg(block._real_offset, os.beg);
    os.write((const char*)&(_cur_data[block._journal_offset]), block._size);
  }
}

void rrdb_journal_file::apply_journal(const my::filename_t & filename)
{
  CHECK_AND_THROW(filename);
  LOG(log::LEVEL_DEBUG2, "Applying journal file for filename '%s' to file '%s'", _cur_filename.get(), filename->c_str());

  // open real file and seek to the start of the file
  rrdb_files_cache::fstream_ptr fs(_files_cache->open_file(filename, time(NULL)));
  fs->sync();

  // apply the journal file to the real file
  this->apply_journal(*fs);

  // flush, don't close (this is the file from cache!)
  fs->flush();
  fs->sync();

  LOG(log::LEVEL_DEBUG2, "Applyed journal file for filename '%s' to file '%s'", _cur_filename.get(), filename->c_str());
}

std::string rrdb_journal_file::get_journal_path() const
{
  // TODO: fix jnl file name
  return std::string("/tmp/rrdb_journal.jnl");
}

// read/write internal journal buffer from/to the file
void rrdb_journal_file::load_journal_file()
{
  CHECK_AND_THROW(_cur_filename.empty());
  CHECK_AND_THROW(_cur_data_blocks.empty());

  // open jnl file
  std::string full_path(this->get_journal_path());
  LOG(log::LEVEL_DEBUG2, "Loading journal file at '%s'", full_path.c_str());

  rrdb_files_cache::fstream_ptr fs = rrdb_files_cache::open_file(full_path);

  // read header
  t_journal_file_header header;
  fs->read((char*)&header, sizeof(header));
  CHECK_AND_THROW(header._magic   == RRDB_JOURNAL_FILE_HEADER_MAGIC);
  CHECK_AND_THROW(header._version == RRDB_JOURNAL_FILE_VERSION);
  CHECK_AND_THROW(header._status  == 0);
  CHECK_AND_THROW(header._blocks_size > 0);

  // read filename
  _cur_filename.read(*fs);

  // read blocks
  my::size_t journal_offset = 0;
  for(my::size_t ii = 0; ii < header._blocks_size; ++ii) {
      // read block header
      t_journal_block_header block_header;
      fs->read((char*)&block_header, sizeof(block_header));
      CHECK_AND_THROW(block_header._magic == RRDB_JOURNAL_BLOCK_HEADER_MAGIC);
      CHECK_AND_THROW(block_header._journal_offset == journal_offset);
      CHECK_AND_THROW(journal_offset == _cur_data.size());

      // read block data
      _cur_data.resize(block_header._journal_offset + block_header._size);
      fs->read(&(_cur_data[block_header._journal_offset]), block_header._size);

      // move on
      _cur_data_blocks.push_back(block_header);
      journal_offset += block_header._size;
  }

  LOG(log::LEVEL_DEBUG, "Loaded journal file at '%s' for filename '%s'", full_path.c_str(), _cur_filename.get());
}

void rrdb_journal_file::save_journal_file()
{
  CHECK_AND_THROW(!_cur_filename.empty());
  CHECK_AND_THROW(!_cur_data_blocks.empty());

  LOG(log::LEVEL_DEBUG2, "Saving journal file for filename '%s'", _cur_filename.get());

  // ensure _jnl_ofs open
  if(!_jnl_ofs) {
       std::string full_path(this->get_journal_path());
       _jnl_ofs = rrdb_files_cache::open_file(full_path, std::ios_base::trunc);
  }
  _jnl_ofs->seekg(0, _jnl_ofs->beg);

  // write header
  t_journal_file_header header;
  header._magic         = RRDB_JOURNAL_FILE_HEADER_MAGIC;
  header._version       = RRDB_JOURNAL_FILE_VERSION;
  header._status        = 0;
  header._blocks_size = _cur_data_blocks.size();
  _jnl_ofs->write((const char*)&header, sizeof(header));

  // write filename
  _cur_filename.write(*_jnl_ofs);

  // write blocks
  BOOST_FOREACH(const t_journal_block_header & block_header, _cur_data_blocks) {
    CHECK_AND_THROW(block_header._journal_offset + block_header._size <= _cur_data.size());
    // write block header
    _jnl_ofs->write((const char*)&block_header, sizeof(block_header));

    // write block data
    _jnl_ofs->write(&(_cur_data[block_header._journal_offset]), block_header._size);
  }

  // flush and bug DO NOT close
  _jnl_ofs->flush();

  // done
  LOG(log::LEVEL_DEBUG, "Saved journal file for filename '%s'", _cur_filename.get());
}

void rrdb_journal_file::delete_journal_file()
{
  LOG(log::LEVEL_DEBUG2, "Deleting journal file");

  // close the
  if(_jnl_ofs) {
      _jnl_ofs->flush();
      _jnl_ofs->close();
      _jnl_ofs.reset();
  }

  std::string full_path(this->get_journal_path());
  boost::filesystem::remove(full_path);

  LOG(log::LEVEL_DEBUG, "Deleted journal file '%s'", full_path.c_str());
}
