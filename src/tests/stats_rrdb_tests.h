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

#define TEST_SUBTEST_START( n, name, verbose ) \
    std::cout << "=== TEST " << n << ": Checking " << name << " ..." << std::endl ; \
    bool test_ok = true; \
    bool test_verbose = (verbose) ;


#define TEST_SUBTEST_END() \
    ++test_cases_total_count; \
    if(!test_ok) { \
      ++test_cases_bad_count; \
      std::cout << "ERROR" << std::endl; \
    } else { \
      std::cout << "OK" << std::endl; \
    }

#define TEST_SUBTEST_END2( msg ) \
    ++test_cases_total_count; \
    if(!test_ok) { \
      ++test_cases_bad_count; \
      std::cout << "ERROR"  << " - " << msg << std::endl; \
    } else { \
      std::cout << "OK"  << " - " << msg << std::endl; \
    }


#define TEST_DATA( name, data ) \
    if(!test_ok) { \
        std::cout << "=== Result for " << name << std::endl; \
        std::cout << data << std::endl; \
    }

//
#define TEST_CHECK( a ) \
    try { \
        ++test_checks_total_count; \
        if( !( a ) ) { \
            ++test_checks_bad_count; \
            test_ok = false; \
            std::cerr << "TEST CHECK FAILURE:" \
              << " '" << string_truncate((#a)) << "' " \
              << " AT " << __FILE__ << ":" << __LINE__ \
              << std::endl; \
        } else if(test_verbose) { \
              std::cerr << "= TEST OK:" \
                << " '" << string_truncate((#a)) << "' " \
                << std::endl; \
        } \
    } catch(const std::exception & e) { \
        ++test_checks_bad_count; \
        std::cerr << "TEST CHECK EXCEPTION: " \
                    << " '" << string_truncate(e.what()) << "' " \
                    << " AT " << __FILE__ << ":" << __LINE__ \
                    << std::endl; \
    }

#define TEST_CHECK_EQUAL( a, b ) \
    try { \
        ++test_checks_total_count; \
        if( (a) != (b) ) { \
            ++test_checks_bad_count; \
            test_ok = false; \
            std::cerr << "TEST CHECK EQUAL FAILURE: " \
              << " '" << string_truncate((#a)) << "' " \
              << " = " <<  (a)  \
              << " IS NOT EQUAL " \
              << " '" << string_truncate((#b)) << "' " \
              << " = " <<  (b) \
              << " AT " << __FILE__ << ":" << __LINE__ \
              << std::endl; \
        } else if(test_verbose) { \
            std::cerr << "= TEST CHECK EQUAL:" \
              << " '" << string_truncate((#a)) << "' " \
              << " is equal " \
              << " '" << string_truncate((#b)) << "' " \
              << std::endl; \
        } \
  } catch(const std::exception & e) { \
      ++test_checks_bad_count; \
      std::cerr << "TEST CHECK EQUAL EXCEPTION: " \
                  << " '" << string_truncate(e.what()) << "' " \
                  << " AT " << __FILE__ << ":" << __LINE__ \
                  << std::endl; \
  }

#define TEST_CHECK_NOT_EQUAL( a, b ) \
    try { \
        ++test_checks_total_count; \
        if( (a) == (b) ) { \
            ++test_checks_bad_count; \
            test_ok = false; \
            std::cerr << "TEST CHECK NOT EQUAL FAILURE: " \
              << " '" << string_truncate((#a)) << "' " \
              << " = " <<  (a) \
              << " IS EQUAL " \
              << " '" << string_truncate((#b)) << "' " \
              << " = " <<  (b) \
              << " AT " << __FILE__ << ":" << __LINE__ \
              << std::endl; \
        } else if(test_verbose) { \
            std::cerr << "= TEST CHECK NOT EQUAL:" \
              << " '" << string_truncate((#a)) << "' " \
              << " is not equal " \
              << " '" << string_truncate((#b)) << "' " \
              << std::endl; \
        } \
  } catch(const std::exception & e) { \
      ++test_checks_bad_count; \
      std::cerr << "TEST CHECK NOT EQUAL EXCEPTION: " \
                  << " '" << string_truncate(e.what()) << "' " \
                  << " AT " << __FILE__ << ":" << __LINE__ \
                  << std::endl; \
  }

#define TEST_CHECK_THROW( expr, expected ) \
    ++test_checks_total_count; \
    try { \
        expr ; \
        ++test_checks_bad_count; \
        test_ok = false; \
        std::cerr << "TEST CHECK THROW FAILURE: " \
            << " '" << string_truncate((#expr)) << "' " \
            << " DID NOT THROW " \
            << " AT "<< __FILE__ << ":" << __LINE__ \
            << std::endl; \
    } catch(const std::exception & e) { \
        if(std::string(e.what()) != std::string(expected)) { \
           test_ok = false; \
           ++test_checks_bad_count; \
           std::cerr << "= TEST CHECK THROW MESSAGE NOT EQUAL:" \
              << " expected = '" << string_truncate(expected) << "' " \
              << " is not equal " \
              << " actual = '" << string_truncate(e.what()) << "' " \
              << " AT "<< __FILE__ << ":" << __LINE__ \
              << std::endl; \
        } else if(test_verbose) { \
            std::cerr << "= TEST CHECK THROW MESSAGE EQUAL:" \
              << " '" << string_truncate(expected) << "' " \
              << std::endl; \
        } \
    }

//
std::string string_truncate(const std::string & str, const my::size_t & num = 100);

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

// Counters
extern int test_cases_total_count;
extern int test_cases_bad_count;
extern int test_checks_total_count;
extern int test_checks_bad_count;

#endif /* STATS_RRDB_TESTS_H_ */
