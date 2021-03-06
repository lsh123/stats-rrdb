/*
 * rrdb_metric.h
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */

#ifndef RRDB_METRIC_H_
#define RRDB_METRIC_H_

#include <string>
#include <fstream>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/intrusive_ptr.hpp>

#include "rrdb/rrdb.h"
#include "rrdb/rrdb_metric_tuple.h"

#include "common/types.h"
#include "common/utils.h"
#include "common/spinlock.h"
#include "parser/retention_policy.h"
#include "common/enable_intrusive_ptr.h"

// forward
class rrdb;
class rrdb_metric_block;
class statement_select;
class rrdb_files_cache;
class rrdb_journal_file;

//
// RRDB Metric file header format:
//
typedef struct t_rrdb_metric_header_ {
  boost::uint16_t   _magic;             // magic bytes (0x99DB)
  boost::uint16_t   _version;           // version (0x01)
  boost::uint16_t   _status;            // status flags
  boost::uint16_t   _blocks_size;       // number of blocks in the metric (size of _blocks array)

  my::value_t       _last_value;        // the latest value
  my::time_t        _last_value_ts;     // the ts of the latest value
} t_rrdb_metric_header;

//
//
//
class rrdb_metric :
    public enable_intrusive_ptr<rrdb_metric>
{
  friend class rrdb_metric_block;

  enum status {
    Status_Dirty        = 0x01,
    Status_Deleted      = 0x02
  };


public:
  rrdb_metric(const my::filename_t & filename = my::filename_t());
  virtual ~rrdb_metric();

  std::string get_name() const ;
  t_retention_policy get_policy() const;
  void create(const std::string & name, const t_retention_policy & policy);

  //
  // status
  //
  inline bool is_dirty() const
  {
    boost::lock_guard<spinlock> guard(_lock);
    return my::bitmask_check<boost::uint16_t>(_header._status, Status_Dirty);
  }
  inline void set_dirty()
  {
    boost::lock_guard<spinlock> guard(_lock);
    my::bitmask_set<boost::uint16_t>(_header._status, Status_Dirty);
  }
  inline bool is_deleted() const
  {
    boost::lock_guard<spinlock> guard(_lock);
    return my::bitmask_check<boost::uint16_t>(_header._status, Status_Deleted);
  }
  //
  // File operations
  //
  my::filename_t get_filename() const;

  void load_file(
      const boost::shared_ptr<rrdb_files_cache> & files_cache
  );
  void save_file(
      const boost::shared_ptr<rrdb_files_cache> & files_cache
  );
  void delete_file(
      const boost::shared_ptr<rrdb_files_cache> & files_cache,
      const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache
  );

  my::size_t save_dirty_blocks(
      const boost::shared_ptr<rrdb_files_cache> & files_cache,
      const boost::shared_ptr<rrdb_journal_file> & journal_file
  );

  static void load_metrics(
      const boost::shared_ptr<rrdb_files_cache> & files_cache,
      const std::string & path,
      rrdb::t_metrics_map & metrics
  );
  static void create_directories(
      const std::string & path
  );
  static my::filename_t construct_filename(
      const std::string & metric_name
  );

  static std::string normalize_name(
      const std::string & str
  );

  //
  // Operations: update/select
  //
  void update(
      const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache,
      const my::time_t & ts,
      const my::value_t & value
  );
  void select(
      const boost::shared_ptr<rrdb_metric_tuples_cache> & tuples_cache,
      const my::time_t & ts1,
      const my::time_t & ts2,
      rrdb::data_walker & walker
  );

  void get_last_value(my::value_t & value, my::time_t & value_ts) const;

private:
  my::size_t write_header(std::ostream & os) const;
  void read_header(std::istream & is);

private:
  mutable spinlock               _lock;
  t_rrdb_metric_header           _header;
  padded_string                  _name;
  my::filename_t                 _filename;

  std::vector< boost::intrusive_ptr<rrdb_metric_block> > _blocks;
}; // class rrdb_metric

#endif /* RRDB_METRIC_H_ */
