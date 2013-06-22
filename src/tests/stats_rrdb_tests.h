/*
 * stats_rrdb_tests.h
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */

#ifndef STATS_RRDB_TESTS_H_
#define STATS_RRDB_TESTS_H_

#include <string>
#include <map>

#include "common/config.h"
#include "common/memory_buffer.h"

#define TEST_START( name ) \
    std::cout  << std::endl << "===== Starting " << name << " test..."  << std::endl << std::endl;

#define TEST_END( name ) \
    std::cout << "===== Finished " << name << " test" << std::endl << std::endl;

#define TEST_SUBTEST_START( n, name ) \
    std::cout << "=== TEST " << n << ": Checking " << name << " ..." << std::endl ; \
    bool test_ok = true;

#define TEST_SUBTEST_END() \
     std::cout << ((test_ok) ? "OK" : "ERROR!!!") << std::endl;

#define TEST_DATA( name, data ) \
    if(!test_ok) { \
        std::cout << "=== Result for " << name << std::endl; \
        std::cout << data << std::endl; \
    }

//
#define TEST_CHECK( a ) \
    if( !( a ) ) { \
        test_ok = false; \
        std::cerr << "TEST CHECK FAILURE: '" \
          << (#a) \
          << std::endl; \
    }

#define TEST_CHECK_EQUAL( a, b ) \
    if( (a) != (b) ) { \
        test_ok = false; \
        std::cerr << "TEST CHECK EQUAL FAILURE: '" \
          << (#a) << "' = " << (a) << " and '" << (#b) << "' = " << (b) \
          << std::endl; \
    }

#define TEST_CHECK_NOT_EQUAL( a, b ) \
    if( (a) == (b) ) { \
        test_ok = false; \
        std::cerr << "TEST CHECK NOT EQUAL FAILURE: '" \
          << (#a) << "' = " << (a) << " and '" << (#b) << "' = " << (b) \
          << std::endl; \
    }

//
// Config helpers
//
typedef std::map<std::string, std::string> t_test_config_data;
boost::shared_ptr<config> test_setup_config(const std::string & path, const t_test_config_data & data);

//
// CSV data helpers
//
typedef std::vector< std::vector<std::string> > t_test_csv_data;
void test_parse_csv_data(const t_memory_buffer_data & data, t_test_csv_data & res);

#endif /* STATS_RRDB_TESTS_H_ */
