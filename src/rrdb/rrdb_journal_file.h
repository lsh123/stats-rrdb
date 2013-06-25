/*
 * rrdb_journal_file.h
 *
 *  Created on: Jun 25, 2013
 *      Author: aleksey
 */

#ifndef RRDB_JOURNAL_FILE_H_
#define RRDB_JOURNAL_FILE_H_

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <boost/shared_ptr.hpp>

#include "rrdb/rrdb_metric_tuple.h"

#include "common/types.h"
#include "common/spinlock.h"
#include "common/memory_buffer.h"


class rrdb_files_cache;

class rrdb_journal_file
{
  typedef struct t_data_block_ {
    my::size_t _real_offset;
    my::size_t _journal_offset;
    my::size_t _size;
  } t_data_block;

public:
  rrdb_journal_file(const boost::shared_ptr<rrdb_files_cache> & files_cache);
  virtual ~rrdb_journal_file();

  // writing data to the journal;s internal buffer
  std::ostream & begin_file(const std::string & full_path);
  void add_block(const my::size_t & offset, const my::size_t & size);
  void end_file();

  // read/write internal journal buffer from/to the file
  void load_journal_file();
  void save_journal_file();

  // apply current journal buffer data to the output file
  void apply_journal(const my::filename_t & filename);

  // clean state
  void reset();

private:
  void apply_journal(std::fstream & os);

private:
  boost::shared_ptr<rrdb_files_cache> _files_cache;

  std::string                         _cur_full_path;
  t_memory_buffer_data                _cur_data;
  t_memory_buffer                     _cur_data_stream;
  std::vector<t_data_block>           _cur_data_blocks;
};

#endif /* RRDB_JOURNAL_FILE_H_ */
