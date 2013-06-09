/*
 * rrdb_metric.cpp
 *
 *  Created on: Jun 8, 2013
 *      Author: aleksey
 */
#include <stdio.h>



#include <boost/shared_array.hpp>
#include <boost/thread/locks.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include "rrdb_metric.h"
#include "rrdb.h"

#include "log.h"
#include "exception.h"

#define RRDB_METRIC_MAGIC               0xDB99
#define RRDB_METRIC_VERSION             0x01

typedef struct rrdb_metric_header_t_ {
  boost::uint16_t   _magic;
  boost::uint8_t    _version;
  boost::uint8_t    _status;
  boost::uint16_t   _name_len;
  boost::uint16_t   _policy_size;
} rrdb_metric_header_t;

// make it configurable?
#define RRDB_METRIC_SUBFOLDERS_NUM      512

rrdb_metric::rrdb_metric() :
    _status(0)
{

}

rrdb_metric::rrdb_metric(const std::string & name, const retention_policy & policy) :
    _name(name),
    _policy(policy),
    _status(0)
{
}

rrdb_metric::~rrdb_metric()
{
  // TODO Auto-generated destructor stub
}

// use copy here to avoid problems with MT
std::string rrdb_metric::get_name()
{
  boost::lock_guard<spinlock> guard(_lock);
  return _name;
}

// use copy here to avoid problems with MT
retention_policy rrdb_metric::get_policy()
{
  boost::lock_guard<spinlock> guard(_lock);
  return _policy;
}

bool rrdb_metric::is_dirty()
{
  boost::lock_guard<spinlock> guard(_lock);
  return (_status & Status_Dirty);
}

void rrdb_metric::set_dirty()
{
  boost::lock_guard<spinlock> guard(_lock);
  _status |= Status_Dirty;
}

bool rrdb_metric::is_deleted()
{
  boost::lock_guard<spinlock> guard(_lock);
  return (_status & Status_Deleted);
}

void rrdb_metric::set_deleted()
{
  boost::lock_guard<spinlock> guard(_lock);
  _status |= Status_Deleted;
}


std::string rrdb_metric::get_full_path(const std::string & folder, const std::string & name)
{
  // calculate subfolder
  std::size_t name_hash = boost::hash<std::string>()(name) % RRDB_METRIC_SUBFOLDERS_NUM;
  char buf[64];
  snprintf(buf, sizeof(buf), "%zu", name_hash);
  std::string subfolder = folder + "/" + buf;

  // ensure folders exist
  boost::filesystem::create_directories(subfolder);

  // the name should match the "a-zA-Z0-9._-" pattern so we can safely
  // use it as filename
  return subfolder + "/" + name + RRDB_METRIC_EXTENSION;
}

// align by 64 bits = 8 bytes
std::size_t rrdb_metric::get_aligned_name_len(std::size_t name_len)
{
  return name_len + (8 - (name_len % 8));
}

void rrdb_metric::save_file(const std::string & folder)
{
  // check if deleted meantime
  if(this->is_deleted()) {
      return;
  }

  // start
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' saving file", this->get_name().c_str());

  // open file
  std::string full_path = rrdb_metric::get_full_path(folder, _name);
  std::fstream ofs(full_path.c_str(), std::ios_base::binary | std::ios_base::out);
  ofs.exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs

  this->write_header(ofs);
  ofs.flush();
  ofs.close();

  // done
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' saved file '%s'", this->get_name().c_str(), full_path.c_str());

  // check if deleted meantime
  if(this->is_deleted()) {
      this->delete_file(folder);
  }
}

boost::shared_ptr<rrdb_metric> rrdb_metric::load_file(const std::string & folder, const std::string & name)
{
  // start
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' loading file", name.c_str());

  // open file
  std::string full_path = rrdb_metric::get_full_path(folder, name);
  std::fstream ifs(full_path.c_str(), std::ios_base::binary | std::ios_base::in);
  ifs.exceptions(std::ifstream::failbit | std::ifstream::failbit); // throw exceptions when error occurs

  boost::shared_ptr<rrdb_metric> res(new rrdb_metric());
  res->read_header(ifs);
  ifs.flush();
  ifs.close();

  if(res->_name != name) {
      throw exception("Unexpected rrdb metric name: %s", res->_name.c_str());
  }

  // done
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' loaded file '%s'", name.c_str(), full_path.c_str());
  return res;
}

void rrdb_metric::delete_file(const std::string & folder)
{
  // mark as deleted in case the flush thread picks it up in the meantime
  this->set_deleted();

  // start
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' deleting file", this->get_name().c_str());

  std::string full_path = rrdb_metric::get_full_path(folder, _name);
  boost::filesystem::remove(full_path);

  // done
  log::write(log::LEVEL_DEBUG, "RRDB metric '%s' deleted file '%s'", this->get_name().c_str(), full_path.c_str());
}

void rrdb_metric::write_header(std::fstream & ofs)
{

  rrdb_metric_header_t header;
  std::size_t aligned_name_size;
  boost::shared_array<char> name;
  std::size_t policy_size;
  boost::shared_array<retention_policy_elem> policy;

  // lock while we are preparing header
  {
    boost::lock_guard<spinlock> guard(_lock);

    header._magic       = RRDB_METRIC_MAGIC;
    header._version     = RRDB_METRIC_VERSION;
    header._status      = _status;
    header._name_len    = _name.length();
    header._policy_size = _policy.size();

    aligned_name_size = rrdb_metric::get_aligned_name_len(header._name_len);
    name.reset(new char[aligned_name_size]);
    memset(name.get(), 0, aligned_name_size);
    memcpy(name.get(), _name.c_str(), header._name_len);

    policy_size = header._policy_size * sizeof(retention_policy_elem);
    policy.reset(new retention_policy_elem[header._policy_size]);
    std::copy(_policy.begin(), _policy.end(), policy.get());
  }

  // write everything
  ofs.write((const char*)&header, sizeof(header));
  ofs.write((const char*)name.get(), aligned_name_size);
  ofs.write((const char*)policy.get(), policy_size);
}

void rrdb_metric::read_header(std::fstream & ifs)
{
  // read header
  rrdb_metric_header_t header;
  ifs.read((char*)&header, sizeof(header));
  if(header._magic != RRDB_METRIC_MAGIC) {
      throw exception("Unexpected rrdb metric magic: %04x", header._magic);
  }
  if(header._version != RRDB_METRIC_VERSION) {
      throw exception("Unexpected rrdb metric version: %04x", header._version);
  }

  // read name
  std::size_t aligned_name_size(rrdb_metric::get_aligned_name_len(header._name_len));
  boost::shared_array<char> name(new char[aligned_name_size]);
  memset(name.get(), 0, aligned_name_size);
  ifs.read((char*)name.get(), aligned_name_size);

  // read policy
  std::size_t policy_size(header._policy_size * sizeof(retention_policy_elem));
  boost::shared_array<retention_policy_elem> policy(new retention_policy_elem[header._policy_size]);
  ifs.read((char*)policy.get(), policy_size);

  // lock while we are updating data
  {
    boost::lock_guard<spinlock> guard(_lock);

    _status = header._status;
    _name.assign(name.get(), header._name_len);
    _policy.assign(policy.get(), policy.get() + header._policy_size);
  }
}
