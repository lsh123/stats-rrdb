/*
 * rrdb_journal_file.h
 *
 *  Created on: Jun 25, 2013
 *      Author: aleksey
 */

#ifndef RRDB_JOURNAL_FILE_H_
#define RRDB_JOURNAL_FILE_H_

#include <string>
#include <fstream>
#include <boost/shared_ptr.hpp>

#include "rrdb/rrdb_metric_tuple.h"

#include "common/types.h"
#include "common/spinlock.h"
#include "common/memory_buffer.h"


class rrdb_files_cache;

class rrdb_journal_file
{
public:
  rrdb_journal_file(const boost::shared_ptr<rrdb_files_cache> & files_cache);
  virtual ~rrdb_journal_file();

  // starts/end writing data to the journal for a given file
  void begin_file(const my::filename_t & filename);
  void end_file();

  // starts writing data block to the currently specified file at a given offset
  std::ostream & begin_file_block(const my::size_t & offset);

  // clean state
  void reset();

private:
  boost::shared_ptr<rrdb_files_cache> _files_cache;
  boost::shared_ptr<std::fstream>     _cur_file;
};

#endif /* RRDB_JOURNAL_FILE_H_ */
