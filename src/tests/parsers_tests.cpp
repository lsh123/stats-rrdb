/*
 * parsers_tests.cpp
 *
 *  Created on: Jun 22, 2013
 *      Author: aleksey
 */

#include "tests/parsers_tests.h"

#include "parser/interval.h"
#include "parser/retention_policy.h"
#include "parser/memory_size.h"
#include "parser/statements.h"

#include "tests/stats_rrdb_tests.h"

#include "common/log.h"
#include "common/config.h"
#include "common/exception.h"

parsers_tests::parsers_tests()
{
}

parsers_tests::~parsers_tests()
{
}

void parsers_tests::run()
{
  parsers_tests test;
  test.test_memory_size(0);
  test.test_interval(1);
  test.test_retention_policy(2);
  test.test_statement_create(3);
  test.test_statement_drop(4);
  test.test_statement_update(5);
  test.test_statement_select(6);
  test.test_statement_show_policy(7);
  test.test_statement_show_metrics(8);
  test.test_statement_show_status(9);

}

void parsers_tests::test_memory_size(const int & n)
{
  TEST_SUBTEST_START(n, "memory_size parser/serializer", false);

  // parser
  TEST_CHECK_EQUAL(memory_size_parse("100"),     100 * MEMORY_SIZE_BYTE);
  TEST_CHECK_EQUAL(memory_size_parse("1 byte"),  1 * MEMORY_SIZE_BYTE);
  TEST_CHECK_EQUAL(memory_size_parse("5 bytes"), 5 * MEMORY_SIZE_BYTE);
  TEST_CHECK_EQUAL(memory_size_parse("4 KB"),    4 * MEMORY_SIZE_KILOBYTE);
  TEST_CHECK_EQUAL(memory_size_parse("4KB"),     4 * MEMORY_SIZE_KILOBYTE);
  TEST_CHECK_EQUAL(memory_size_parse("3 MB"),    3 * MEMORY_SIZE_MEGABYTE);
  TEST_CHECK_EQUAL(memory_size_parse("3MB"),     3 * MEMORY_SIZE_MEGABYTE);
  TEST_CHECK_EQUAL(memory_size_parse("2 GB"),    2 * MEMORY_SIZE_GIGABYTE);
  TEST_CHECK_EQUAL(memory_size_parse("2GB"),     2 * MEMORY_SIZE_GIGABYTE);

  // parser bad strings
  TEST_CHECK_THROW(memory_size_parse(""), "Parser error: expecting <memory size value> at \"\"");
  TEST_CHECK_THROW(memory_size_parse("1 xyz"), "Parser error: 'memory size' unexpected 'xyz'");
  TEST_CHECK_THROW(memory_size_parse("1MB 123"), "Parser error: 'memory size' unexpected '123'");

  // serializer
  TEST_CHECK_EQUAL(memory_size_write(100 * MEMORY_SIZE_BYTE),   "100 bytes");
  TEST_CHECK_EQUAL(memory_size_write(1 * MEMORY_SIZE_BYTE),     "1 byte");
  TEST_CHECK_EQUAL(memory_size_write(5 * MEMORY_SIZE_BYTE),     "5 bytes");
  TEST_CHECK_EQUAL(memory_size_write(4 * MEMORY_SIZE_KILOBYTE), "4KB");
  TEST_CHECK_EQUAL(memory_size_write(3 * MEMORY_SIZE_MEGABYTE), "3MB");
  TEST_CHECK_EQUAL(memory_size_write(2 * MEMORY_SIZE_GIGABYTE), "2GB");

  // done
  TEST_SUBTEST_END();
}

// tests
void parsers_tests::test_interval(const int & n)
{
  TEST_SUBTEST_START(n, "interval parser/serializer", false);

  // parser
  TEST_CHECK_EQUAL(interval_parse("1 sec"),   1 * INTERVAL_SEC);
  TEST_CHECK_EQUAL(interval_parse("5 secs"),  5 * INTERVAL_SEC);
  TEST_CHECK_EQUAL(interval_parse("1 min"),   1 * INTERVAL_MIN);
  TEST_CHECK_EQUAL(interval_parse("12 mins"), 12 * INTERVAL_MIN);
  TEST_CHECK_EQUAL(interval_parse("1 hour"),  1 * INTERVAL_HOUR);
  TEST_CHECK_EQUAL(interval_parse("3 hours"), 3 * INTERVAL_HOUR);
  TEST_CHECK_EQUAL(interval_parse("1 day"),   1 * INTERVAL_DAY);
  TEST_CHECK_EQUAL(interval_parse("2 days"),  2 * INTERVAL_DAY);
  TEST_CHECK_EQUAL(interval_parse("1 week"),  1 * INTERVAL_WEEK);
  TEST_CHECK_EQUAL(interval_parse("2 weeks"), 2 * INTERVAL_WEEK);
  TEST_CHECK_EQUAL(interval_parse("1 month"), 1 * INTERVAL_MONTH);
  TEST_CHECK_EQUAL(interval_parse("5 months"),5 * INTERVAL_MONTH);
  TEST_CHECK_EQUAL(interval_parse("1 year"),  1 * INTERVAL_YEAR);
  TEST_CHECK_EQUAL(interval_parse("3 years"), 3 * INTERVAL_YEAR);

  // parser bad strings
  TEST_CHECK_THROW(interval_parse(""), "Parser error: expecting <interval value> at \"\"");
  TEST_CHECK_THROW(interval_parse("1"), "Parser error: expecting <interval unit> at \"\"");
  TEST_CHECK_THROW(interval_parse("1 xyz"), "Parser error: expecting <interval unit> at \"xyz\"");
  TEST_CHECK_THROW(interval_parse("1 sec 123"), "Parser error: 'interval' unexpected '123'");

  // serializer
  TEST_CHECK_EQUAL(interval_write(100 * INTERVAL_SEC), "100 secs");
  TEST_CHECK_EQUAL(interval_write(1 * INTERVAL_SEC),   "1 sec");
  TEST_CHECK_EQUAL(interval_write(5 * INTERVAL_SEC),   "5 secs");
  TEST_CHECK_EQUAL(interval_write(1 * INTERVAL_MIN),   "1 min");
  TEST_CHECK_EQUAL(interval_write(12 * INTERVAL_MIN),  "12 mins");
  TEST_CHECK_EQUAL(interval_write(1 * INTERVAL_HOUR),  "1 hour");
  TEST_CHECK_EQUAL(interval_write(3 * INTERVAL_HOUR),  "3 hours");
  TEST_CHECK_EQUAL(interval_write(1 * INTERVAL_DAY),   "1 day");
  TEST_CHECK_EQUAL(interval_write(2 * INTERVAL_DAY),   "2 days");
  TEST_CHECK_EQUAL(interval_write(1 * INTERVAL_WEEK),  "1 week");
  TEST_CHECK_EQUAL(interval_write(2 * INTERVAL_WEEK),  "2 weeks");
  TEST_CHECK_EQUAL(interval_write(1 * INTERVAL_MONTH), "1 month");
  TEST_CHECK_EQUAL(interval_write(5 * INTERVAL_MONTH), "5 months");
  TEST_CHECK_EQUAL(interval_write(1 * INTERVAL_YEAR),  "1 year");
  TEST_CHECK_EQUAL(interval_write(3 * INTERVAL_YEAR),  "3 years");

  // done
  TEST_SUBTEST_END();
}

void parsers_tests::test_retention_policy(const int & n)
{
  TEST_SUBTEST_START(n, "retention_policy parser/serializer", false);
  t_retention_policy rp;

  // parser
  rp = retention_policy_parse("1 sec for 3 min");
  TEST_CHECK_EQUAL(rp.size(), 1);
  TEST_CHECK_EQUAL(rp[0]._freq,     1 * INTERVAL_SEC);
  TEST_CHECK_EQUAL(rp[0]._duration, 3 * INTERVAL_MIN);

  rp = retention_policy_parse("1 sEc foR 3 MiN");
  TEST_CHECK_EQUAL(rp.size(), 1);
  TEST_CHECK_EQUAL(rp[0]._freq,     1 * INTERVAL_SEC);
  TEST_CHECK_EQUAL(rp[0]._duration, 3 * INTERVAL_MIN);

  rp = retention_policy_parse("2 hours for 1 day, 6 hours for 3 months");
  TEST_CHECK_EQUAL(rp.size(), 2);
  TEST_CHECK_EQUAL(rp[0]._freq,     2 * INTERVAL_HOUR);
  TEST_CHECK_EQUAL(rp[0]._duration, 1 * INTERVAL_DAY);
  TEST_CHECK_EQUAL(rp[1]._freq,     6 * INTERVAL_HOUR);
  TEST_CHECK_EQUAL(rp[1]._duration, 3 * INTERVAL_MONTH);

  // parser bad strings
  TEST_CHECK_THROW(retention_policy_parse(""), "Parser error: expecting <interval value> at \"\"");
  TEST_CHECK_THROW(retention_policy_parse("1"), "Parser error: expecting <interval unit> at \"\"");
  TEST_CHECK_THROW(retention_policy_parse("1 sec for"), "Parser error: expecting <interval value> at \"\"");
  TEST_CHECK_THROW(retention_policy_parse("1 sec for ,"), "Parser error: expecting <interval value> at \",\"");
  TEST_CHECK_THROW(retention_policy_parse("1 sec for 2,"), "Parser error: expecting <interval unit> at \",\"");
  TEST_CHECK_THROW(retention_policy_parse("1 sec for 2 mins,"), "Parser error: expecting <interval value> at \"\"");
  TEST_CHECK_THROW(retention_policy_parse("1 sec for 2 mins 123"), "Parser error: 'retention policy' unexpected '1 sec for 2 mins 123'");
  TEST_CHECK_THROW(retention_policy_parse("1 sec for 2 mins, 123"), "Parser error: expecting <interval unit> at \"\"");
  TEST_CHECK_THROW(retention_policy_parse("1 sec for 2 mins, abc"), "Parser error: expecting <interval value> at \"abc\"");

  // serializer
  rp.clear();
  rp.push_back(t_retention_policy_elem(2 * INTERVAL_SEC, 10 * INTERVAL_MIN));
  TEST_CHECK_EQUAL(retention_policy_write(rp), "2 secs for 10 mins");

  rp.clear();
  rp.push_back(t_retention_policy_elem(3 * INTERVAL_HOUR, 2 * INTERVAL_DAY));
  rp.push_back(t_retention_policy_elem(6 * INTERVAL_HOUR, 1 * INTERVAL_YEAR));
  TEST_CHECK_EQUAL(retention_policy_write(rp), "3 hours for 2 days, 6 hours for 1 year");

  // done
  TEST_SUBTEST_END();
}

void parsers_tests::test_statement_create(const int & n)
{
  TEST_SUBTEST_START(n, "statement CREATE METRIC", false);
  t_statement vst;
  statement_create st;

  // full
  vst = statement_query_parse("CREATE METRIC 'test' KEEP 3 sec for 2 days; ");
  st = boost::get<statement_create>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");
  TEST_CHECK_EQUAL(st._policy.size(), 1);
  TEST_CHECK_EQUAL(st._policy[0]._freq,     3 * INTERVAL_SEC);
  TEST_CHECK_EQUAL(st._policy[0]._duration, 2 * INTERVAL_DAY);

  // different quotes
  vst = statement_query_parse("CREATE METRIC \"test\" KEEP 3 sec for 2 days; ");
  st = boost::get<statement_create>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");
  TEST_CHECK_EQUAL(st._policy.size(), 1);
  TEST_CHECK_EQUAL(st._policy[0]._freq,     3 * INTERVAL_SEC);
  TEST_CHECK_EQUAL(st._policy[0]._duration, 2 * INTERVAL_DAY);

  // mixed case
  vst = statement_query_parse("cReate meTrIc 'test' kEEp 3 SEC fOr 2 DaYs; ");
  st = boost::get<statement_create>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");
  TEST_CHECK_EQUAL(st._policy.size(), 1);
  TEST_CHECK_EQUAL(st._policy[0]._freq,     3 * INTERVAL_SEC);
  TEST_CHECK_EQUAL(st._policy[0]._duration, 2 * INTERVAL_DAY);

  // no "metric"
  vst = statement_query_parse("CREATE 'test' KEEP 3 sec for 2 days; ");
  st = boost::get<statement_create>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");
  TEST_CHECK_EQUAL(st._policy.size(), 1);
  TEST_CHECK_EQUAL(st._policy[0]._freq,     3 * INTERVAL_SEC);
  TEST_CHECK_EQUAL(st._policy[0]._duration, 2 * INTERVAL_DAY);

  // TODO: implement optional retention policy and test it here

  // errors
  TEST_CHECK_THROW(statement_query_parse("CREATE"), "Parser error: expecting <quoted metric name> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("CREATE xxx"), "Parser error: expecting <quoted metric name> at \"xxx\"");
  TEST_CHECK_THROW(statement_query_parse("CREATE METRIC"), "Parser error: expecting <quoted metric name> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("CREATE METRIC KEEP"), "Parser error: expecting <quoted metric name> at \" KEEP\"");
  TEST_CHECK_THROW(statement_query_parse("CREATE METRIC 'test' KEEP"), "Parser error: expecting <interval value> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("CREATE METRIC 'test' KEEP 3 sec"), "Parser error: expecting \"for\" at \"\"");
  TEST_CHECK_THROW(statement_query_parse("CREATE METRIC 'test' KEEP 3 sec for 2 days"), "Parser error: expecting \";\" at \"\"");
  TEST_CHECK_THROW(statement_query_parse("CREATE METRIC 'test' KEEP 3 sec for 2 days,"), "Parser error: expecting <interval value> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("CREATE METRIC 'test' KEEP 3 sec for 2 days; xxx"), "Parser error: 'query statement' unexpected  'xxx'");

  // done
  TEST_SUBTEST_END();
}

void parsers_tests::test_statement_drop(const int & n)
{
  TEST_SUBTEST_START(n, "statement DROP METRIC", false);
  t_statement vst;
  statement_drop st;

  // full
  vst = statement_query_parse("DROP METRIC 'test' ; ");
  st = boost::get<statement_drop>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");

  // mixed case
  vst = statement_query_parse("Drop mETric 'test';");
  st = boost::get<statement_drop>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");

  // no metric
  vst = statement_query_parse("DROP 'test' ; ");
  st = boost::get<statement_drop>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");

  // errors
  TEST_CHECK_THROW(statement_query_parse("DROP"), "Parser error: expecting <quoted metric name> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("DROP METRIC"), "Parser error: expecting <quoted metric name> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("DROP METRIC 'test'"), "Parser error: expecting \";\" at \"\"");
  TEST_CHECK_THROW(statement_query_parse("DROP METRIC 'test';xxx"), "Parser error: 'query statement' unexpected  'xxx'");

  // done
  TEST_SUBTEST_END();
}

void parsers_tests::test_statement_update(const int & n)
{
  TEST_SUBTEST_START(n, "statement UPDATE METRIC", false);
  t_statement vst;
  statement_update st;

  // full
  vst = statement_query_parse("UPDATE METRIC 'test' ADD 1.0 AT 123456789; ");
  st = boost::get<statement_update>(vst);
  TEST_CHECK_EQUAL(st._name,     "test");
  TEST_CHECK_EQUAL(st._value,     1.0);
  TEST_CHECK(st._ts);
  TEST_CHECK_EQUAL(st._ts.get(),  123456789);

  // mixed case
  vst = statement_query_parse("uPDatE MeTRiC 'test' add 1.0 at 123456789; ");
  st = boost::get<statement_update>(vst);
  TEST_CHECK_EQUAL(st._name,     "test");
  TEST_CHECK_EQUAL(st._value,     1.0);
  TEST_CHECK(st._ts);
  TEST_CHECK_EQUAL(st._ts.get(),  123456789);

  // no "metric"
  vst = statement_query_parse("UPDATE 'test' ADD 1.0 AT 123456789; ");
  st = boost::get<statement_update>(vst);
  TEST_CHECK_EQUAL(st._name,     "test");
  TEST_CHECK_EQUAL(st._value,     1.0);
  TEST_CHECK(st._ts);
  TEST_CHECK_EQUAL(st._ts.get(),  123456789);

  // no "AT ...."
  vst = statement_query_parse("UPDATE METRIC 'test' ADD 1.0 ; ");
  st = boost::get<statement_update>(vst);
  TEST_CHECK_EQUAL(st._name,     "test");
  TEST_CHECK_EQUAL(st._value,     1.0);
  TEST_CHECK(!st._ts);

  // no "metric" and "AT ...."
  vst = statement_query_parse("UPDATE 'test' ADD 1.0 ; ");
  st = boost::get<statement_update>(vst);
  TEST_CHECK_EQUAL(st._name,     "test");
  TEST_CHECK_EQUAL(st._value,     1.0);
  TEST_CHECK(!st._ts);

  // errors
  TEST_CHECK_THROW(statement_query_parse("UPDATE"), "Parser error: expecting <quoted metric name> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("UPDATE METRIC"), "Parser error: expecting <quoted metric name> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("UPDATE METRIC 'test'"), "Parser error: expecting \"add\" at \"\"");
  TEST_CHECK_THROW(statement_query_parse("UPDATE METRIC 'test' ADD"), "Parser error: expecting <real> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("UPDATE METRIC 'test' ADD 1.0"), "Parser error: expecting \";\" at \"\"");
  TEST_CHECK_THROW(statement_query_parse("UPDATE METRIC 'test' ADD 1.0 AT"), "Parser error: expecting <unsigned-integer> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("UPDATE METRIC 'test' ADD 1.0 AT 123456789;xxx"), "Parser error: 'query statement' unexpected  'xxx'");


  // done
  TEST_SUBTEST_END();
}

void parsers_tests::test_statement_select(const int & n)
{
  TEST_SUBTEST_START(n, "statement SELECT FROM METRIC", false);
  t_statement vst;
  statement_select st;

  // TODO: support no BETWEEN and write test for it

  // full
  vst = statement_query_parse("SELECT * FROM METRIC 'test' BETWEEN 0 and 123456789 GROUP BY 5 mins; ");
  st = boost::get<statement_select>(vst);
  TEST_CHECK_EQUAL(st._name,           "test");
  TEST_CHECK_EQUAL(st._ts_begin,        0);
  TEST_CHECK_EQUAL(st._ts_end,          123456789);
  TEST_CHECK(st._group_by);
  TEST_CHECK_EQUAL(st._group_by.get(),  5 * INTERVAL_MIN);

  // mixed case
  vst = statement_query_parse("selEct * frOm metrIc 'test' beTWeen 0 aNd 123456789 grOUp By 5 miNs; ");
  st = boost::get<statement_select>(vst);
  TEST_CHECK_EQUAL(st._name,           "test");
  TEST_CHECK_EQUAL(st._ts_begin,        0);
  TEST_CHECK_EQUAL(st._ts_end,          123456789);
  TEST_CHECK(st._group_by);
  TEST_CHECK_EQUAL(st._group_by.get(),  5 * INTERVAL_MIN);

  // no "metric"
  vst = statement_query_parse("SELECT * FROM 'test' BETWEEN 0 and 123456789 GROUP BY 5 mins; ");
  st = boost::get<statement_select>(vst);
  TEST_CHECK_EQUAL(st._name,           "test");
  TEST_CHECK_EQUAL(st._ts_begin,        0);
  TEST_CHECK_EQUAL(st._ts_end,          123456789);
  TEST_CHECK(st._group_by);
  TEST_CHECK_EQUAL(st._group_by.get(),  5 * INTERVAL_MIN);

  // no GROUP BY
  vst = statement_query_parse("SELECT * FROM METRIC 'test' BETWEEN 0 and 123456789 ; ");
  st = boost::get<statement_select>(vst);
  TEST_CHECK_EQUAL(st._name,           "test");
  TEST_CHECK_EQUAL(st._ts_begin,        0);
  TEST_CHECK_EQUAL(st._ts_end,          123456789);
  TEST_CHECK(!st._group_by);

  // no "metric" and no GROUP BY
  vst = statement_query_parse("SELECT * FROM 'test' BETWEEN 0 and 123456789; ");
  st = boost::get<statement_select>(vst);
  TEST_CHECK_EQUAL(st._name,           "test");
  TEST_CHECK_EQUAL(st._ts_begin,        0);
  TEST_CHECK_EQUAL(st._ts_end,          123456789);
  TEST_CHECK(!st._group_by);

  // errors
  TEST_CHECK_THROW(statement_query_parse("SELECT"), "'Parser error: expecting \"*\" at \"\"");
  TEST_CHECK_THROW(statement_query_parse("SELECT FROM"), "Parser error: expecting \"*\" at \"FROM\"");
  TEST_CHECK_THROW(statement_query_parse("SELECT * FROM 'test'"), "Parser error: expecting \"add\" at \"\"");
  TEST_CHECK_THROW(statement_query_parse("SELECT * FROM 'test' BETWEEN"), "Parser error: expecting <unsigned-integer> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("SELECT * FROM 'test' BETWEEN 0"), "Parser error: expecting \"and\" at \"\"");
  TEST_CHECK_THROW(statement_query_parse("SELECT * FROM 'test' BETWEEN 0 and"), "Parser error: expecting <unsigned-integer> at \"\"");
  TEST_CHECK_THROW(statement_query_parse("SELECT * FROM 'test' BETWEEN 0 and 123456;xxx"), "Parser error: 'query statement' unexpected  'xxx'");

  // done
  TEST_SUBTEST_END();
}

void parsers_tests::test_statement_show_policy(const int & n)
{
  TEST_SUBTEST_START(n, "statement SHOW METRIC POLICY", false);
  t_statement vst;
  statement_show_policy st;

  // full
  vst = statement_query_parse("SHOW METRIC POLICY 'test' ; ");
  st = boost::get<statement_show_policy>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");

  // mixed case
  vst = statement_query_parse("sHoW metrIc PolIcy 'test' ; ");
  st = boost::get<statement_show_policy>(vst);
  TEST_CHECK_EQUAL(st._name,  "test");

  // done
  TEST_SUBTEST_END();
}

void parsers_tests::test_statement_show_metrics(const int & n)
{
  TEST_SUBTEST_START(n, "statement SHOW METRICS", false);
  t_statement vst;
  statement_show_metrics st;

  // full
  vst = statement_query_parse("SHOW METRICS LIKE 'test';");
  st = boost::get<statement_show_metrics>(vst);
  TEST_CHECK(st._like);
  TEST_CHECK_EQUAL(st._like.get(), "test");

  // mixed case
  vst = statement_query_parse("sHow mETRics LiKe 'test';");
  st = boost::get<statement_show_metrics>(vst);
  TEST_CHECK(st._like);
  TEST_CHECK_EQUAL(st._like.get(), "test");

  // done
  TEST_SUBTEST_END();
}

void parsers_tests::test_statement_show_status(const int & n)
{
  TEST_SUBTEST_START(n, "statement SHOW STATUS", false);
  t_statement vst;
  statement_show_status st;

  // full
  vst = statement_query_parse("SHOW STATUS LIKE 'test'; ");
  st = boost::get<statement_show_status>(vst);
  TEST_CHECK(st._like);
  TEST_CHECK_EQUAL(st._like.get(), "test");

  // mixed case
  vst = statement_query_parse("shOW StAtUs lIkE 'test'; ");
  st = boost::get<statement_show_status>(vst);
  TEST_CHECK(st._like);
  TEST_CHECK_EQUAL(st._like.get(), "test");

  // done
  TEST_SUBTEST_END();
}

