/*
 * lru_tests.cpp
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */

#include "tests/lru_tests.h"
#include "tests/stats_rrdb_tests.h"

#include "common/log.h"
#include "common/config.h"
#include "common/exception.h"



lru_tests::lru_tests():
  _start_ts(time(NULL))
{
}

lru_tests::~lru_tests()
{
}

void lru_tests::test0_insert()
{
  TEST_SUBTEST_START(0, "insert()");

  // insert some data
  _cache.insert("test-0", 0, _start_ts);
  _cache.insert("test-1", 1, _start_ts + 1);
  _cache.insert("test-2", 2, _start_ts + 2);

  // check size
  my::size_t size = _cache.size();
  TEST_CHECK_EQUAL(size, 3);

  // check it's in the right order
  t_test_lru_cache::t_lru_iterator lru_it(_cache.lru_begin());
  t_test_lru_cache::value_type vt(*(lru_it++));

  TEST_CHECK(lru_it != _cache.lru_end());
  TEST_CHECK_EQUAL(vt._v, 0);
  TEST_CHECK_EQUAL(vt._k, "test-0");
  vt = *(lru_it++);

  TEST_CHECK(lru_it != _cache.lru_end());
  TEST_CHECK_EQUAL(vt._v, 1);
  TEST_CHECK_EQUAL(vt._k, "test-1");
  vt = *(lru_it++);

  TEST_CHECK(lru_it == _cache.lru_end());
  TEST_CHECK_EQUAL(vt._v, 2);
  TEST_CHECK_EQUAL(vt._k, "test-2");

  // done
  TEST_SUBTEST_END();
}

void lru_tests::test1_find_and_use()
{
  TEST_SUBTEST_START(1, "find() and use()");

  // check size
  my::size_t size = _cache.size();
  TEST_CHECK_EQUAL(size, 3);

  t_test_lru_cache::t_iterator it(_cache.find("test-1"));
  TEST_CHECK(it != _cache.end());
  _cache.use(it, _start_ts + 10);

  // the "test-1" key should now be the most recently used
  t_test_lru_cache::t_lru_iterator lru_it(_cache.lru_begin());
  t_test_lru_cache::value_type vt(*(lru_it++));

  TEST_CHECK(lru_it != _cache.lru_end());
  TEST_CHECK_EQUAL(vt._v, 0);
  TEST_CHECK_EQUAL(vt._k, "test-0");
  vt = *(lru_it++);

  TEST_CHECK(lru_it != _cache.lru_end());
  TEST_CHECK_EQUAL(vt._v, 2);
  TEST_CHECK_EQUAL(vt._k, "test-2");
  vt = *(lru_it++);

  TEST_CHECK(lru_it == _cache.lru_end());
  TEST_CHECK_EQUAL(vt._v, 1);
  TEST_CHECK_EQUAL(vt._k, "test-1");

  // done
  TEST_SUBTEST_END();
}

void lru_tests::test2_erase()
{
  TEST_SUBTEST_START(2, "erase()");

  // check size
  my::size_t size = _cache.size();
  TEST_CHECK_EQUAL(size, 3);

  // erase the second lru item
  t_test_lru_cache::t_lru_iterator lru_it(_cache.lru_begin());
  t_test_lru_cache::value_type vt(*(++lru_it));
  TEST_CHECK_EQUAL(vt._v, 2);
  TEST_CHECK_EQUAL(vt._k, "test-2");
  lru_it = _cache.lru_erase(lru_it);

  // check size
  size = _cache.size();
  TEST_CHECK_EQUAL(size, 2);

  // it should now point to the next item
  TEST_CHECK(lru_it != _cache.lru_end());
  vt = *(lru_it++);

  TEST_CHECK(lru_it == _cache.lru_end());
  TEST_CHECK_EQUAL(vt._v, 1);
  TEST_CHECK_EQUAL(vt._k, "test-1");

  // done
  TEST_SUBTEST_END();
}

void lru_tests::run()
{
  lru_tests test;
  test.test0_insert();
  test.test1_find_and_use();
  test.test2_erase();
}
