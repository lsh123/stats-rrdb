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
  _files_cache(files_cache)
{
}

rrdb_journal_file::~rrdb_journal_file()
{
}

// clean state
void rrdb_journal_file::reset()
{
  _cur_file.reset();
}

// starts/end writing data to the journal for a given file
void rrdb_journal_file::begin_file(const my::filename_t & filename)
{
  CHECK_AND_THROW(filename);
  CHECK_AND_THROW(_files_cache);
  CHECK_AND_THROW(!_cur_file);


  // open file and seek to the start of the file
  _cur_file = _files_cache->open_file(filename, time(NULL));
  _cur_file->sync();
}

void rrdb_journal_file::end_file()
{
  CHECK_AND_THROW(_cur_file);

  // flush, don't close (this is the file from cache!)
  _cur_file->flush();
  _cur_file->sync();

  // cleanup for the next one
  this->reset();
}

// starts/ends writing data block to the currently specified file at a given offset
std::ostream & rrdb_journal_file::begin_file_block(const my::size_t & offset)
{
  CHECK_AND_THROW(_cur_file);

  _cur_file->seekg(offset, _cur_file->beg);
  return (*_cur_file);
}



