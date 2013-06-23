/*
 * files_cache_tests.cpp
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */

#include "rrdb/rrdb_files_cache.h"

#include "common/log.h"
#include "common/config.h"
#include "common/exception.h"

#include "tests/files_cache_tests.h"
#include "tests/stats_rrdb_tests.h"

#define FILENAME_TEMPLATE       "test_file_%d.tst"
#define FILENAME_MAX_NUM        10


files_cache_tests::files_cache_tests() :
  _files_cache(new rrdb_files_cache())
{
}


files_cache_tests::~files_cache_tests()
{
}

void files_cache_tests::initialize(const std::string & path)
{
  // create config
  t_test_config_data config_data;
  config_data["rrdb.path"]                  = path;
  config_data["rrdb.open_files_cache_size"] = "2";
  config_data["rrdb.open_files_cache_purge_threshold"] = "1.0"; // make sure we kick out at most one
  config_data["log.level"]                  = "debug3";
  config_data["log.destination"]            = path + "./files_cache_tests.log";

  //
  boost::shared_ptr<config> cfg = test_setup_config(path, config_data);

  // initiliaze
  _files_cache->initialize(cfg);
}

my::filename_t files_cache_tests::get_filename(int n)
{
  CHECK_AND_THROW(n < FILENAME_MAX_NUM);

  char buf[1024];
  snprintf(buf, sizeof(buf), FILENAME_TEMPLATE, n);
  my::filename_t res(new std::string(buf));
  return res;
}

void files_cache_tests::cleanup()
{
  // ensure files are not there
  for(int ii = 0; ii < FILENAME_MAX_NUM; ++ii) {
      _files_cache->delete_file(files_cache_tests::get_filename(ii));
  }
}

void files_cache_tests::test_open_file(const int & n)
{
  TEST_SUBTEST_START(n, "open_file");

  my::time_t ts = time(NULL);
  my::size_t size = 2; // this is what we set in init
  my::size_t hits = 0;
  my::size_t misses = 0;

  // insert two files, both should be misses
  _files_cache->open_file(files_cache_tests::get_filename(1), ts + 0, true);
  ++misses;
  _files_cache->open_file(files_cache_tests::get_filename(2), ts + 1, true);
  ++misses;
  _files_cache->open_file(files_cache_tests::get_filename(3), ts + 2, true);
  ++misses;
  TEST_CHECK_EQUAL(_files_cache->get_cache_size(),   size);
  TEST_CHECK_EQUAL(_files_cache->get_cache_hits(),   hits);
  TEST_CHECK_EQUAL(_files_cache->get_cache_misses(), misses);

  // insert the file #2 and #3 again: both should be in the cache... this time
  // we put later timestamp on #2
  _files_cache->open_file(files_cache_tests::get_filename(3), ts + 3, true);
  ++hits;
  _files_cache->open_file(files_cache_tests::get_filename(2), ts + 4, true);
  ++hits;
  TEST_CHECK_EQUAL(_files_cache->get_cache_size(),   size);
  TEST_CHECK_EQUAL(_files_cache->get_cache_hits(),   hits);
  TEST_CHECK_EQUAL(_files_cache->get_cache_misses(), misses);

  // now if we insert another one, then it should kick out #3 since we "used" #2 more recently
  _files_cache->open_file(files_cache_tests::get_filename(4), ts + 5, true);
  ++misses;
  TEST_CHECK_EQUAL(_files_cache->get_cache_size(),   size);
  TEST_CHECK_EQUAL(_files_cache->get_cache_hits(),   hits);
  TEST_CHECK_EQUAL(_files_cache->get_cache_misses(), misses);

  // #2 should be in the cache
  _files_cache->open_file(files_cache_tests::get_filename(2), ts + 6, true);
  ++hits;
  TEST_CHECK_EQUAL(_files_cache->get_cache_size(),   size);
  TEST_CHECK_EQUAL(_files_cache->get_cache_hits(),   hits);
  TEST_CHECK_EQUAL(_files_cache->get_cache_misses(), misses);

  // #3 should've been kicked from the cache
  _files_cache->open_file(files_cache_tests::get_filename(3), ts + 7, true);
  ++misses;
  TEST_CHECK_EQUAL(_files_cache->get_cache_size(),   size);
  TEST_CHECK_EQUAL(_files_cache->get_cache_hits(),   hits);
  TEST_CHECK_EQUAL(_files_cache->get_cache_misses(), misses);

  // done
  TEST_SUBTEST_END();
}

void files_cache_tests::test_delete_file(const int & n)
{
  TEST_SUBTEST_START(n, "delete_file");

  // should have full cache (2) from previous test
  TEST_CHECK_EQUAL(_files_cache->get_cache_size(), 2);

  // should have had #3 from previous test in the cache
  _files_cache->delete_file(files_cache_tests::get_filename(3));
  TEST_CHECK_EQUAL(_files_cache->get_cache_size(), 1);

  // done
  TEST_SUBTEST_END();
}

void files_cache_tests::run(const std::string & path)
{
  // setup
  files_cache_tests test;
  test.initialize(path);
  test.cleanup();

  // tests
  test.test_open_file(0);
  test.test_delete_file(0);

  // cleanup
  test.cleanup();
}

