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

#include "common/utils.h"
#include "common/types.h"
#include "common/spinlock.h"
#include "common/memory_buffer.h"


class rrdb_files_cache;


typedef struct t_journal_file_header_ {
  boost::uint16_t   _magic;             // magic bytes (RRDB_JOURNAL_FILE_HEADER_MAGIC)
  boost::uint16_t   _version;           // version (0x01)
  boost::uint16_t   _status;            // status flags
  boost::uint16_t   _blocks_size;       // number of blocks
} t_journal_file_header;

typedef struct t_journal_block_header_ {
   boost::uint16_t   _magic;             // magic bytes (RRDB_JOURNAL_BLOCK_HEADER_MAGIC)
   boost::uint16_t   _unused1;           //
   boost::uint16_t   _unused2;           //
   boost::uint16_t   _unused3;           //

   my::size_t        _real_offset;
   my::size_t        _journal_offset;
   my::size_t        _size;
 } t_journal_block_header;

class rrdb_journal_file
{
public:
  rrdb_journal_file(const boost::shared_ptr<rrdb_files_cache> & files_cache);
  virtual ~rrdb_journal_file();

  // writing data to the journal;s internal buffer
  std::ostream & begin_file(const std::string & filename);
  void add_block(const my::size_t & offset, const my::size_t & size);
  void end_file();

  // read/write internal journal buffer from/to the file
  void load_journal_file();
  void save_journal_file();
  void delete_journal_file();
  std::string get_journal_path() const;

  // apply current journal buffer data to the output file
  void apply_journal(const my::filename_t & filename);

  // clean state
  void clear();

  // data
  inline my::size_t get_blocks_count() const
  {
    return _cur_data_blocks.size();
  }
  inline const char* get_filename() const
  {
    return _cur_filename.get();
  }

  void apply_journal(std::fstream & os);

private:


  boost::shared_ptr<rrdb_files_cache>  _files_cache;

  padded_string                        _cur_filename;
  t_memory_buffer_data                 _cur_data;
  t_memory_buffer                      _cur_data_stream;
  std::vector<t_journal_block_header>  _cur_data_blocks;

  boost::shared_ptr<std::fstream>      _jnl_ofs;
};

#endif /* RRDB_JOURNAL_FILE_H_ */
