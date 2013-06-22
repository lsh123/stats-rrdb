/*
 * stats_rrdb_tests.h
 *
 *  Created on: Jun 21, 2013
 *      Author: aleksey
 */

#ifndef STATS_RRDB_TESTS_H_
#define STATS_RRDB_TESTS_H_

#define TEST_CHECK( a ) \
    if( !( a ) ) { \
        std::cerr << "TEST CHECK FAILURE: '" \
          << (#a) \
          << std::endl; \
    }

#define TEST_CHECK_EQUAL( a, b ) \
    if( (a) != (b) ) { \
        std::cerr << "TEST CHECK EQUAL FAILURE: '" \
          << (#a) << "' = " << (a) << " and '" << (#b) << "' = " << (b) \
          << std::endl; \
    }

#define TEST_CHECK_NOT_EQUAL( a, b ) \
    if( (a) == (b) ) { \
        std::cerr << "TEST CHECK NOT EQUAL FAILURE: '" \
          << (#a) << "' = " << (a) << " and '" << (#b) << "' = " << (b) \
          << std::endl; \
    }

#define TEST_START( name ) \
    std::cout << "===== Starting " << name << " test..." << std::endl;

#define TEST_END( name ) \
    std::cout << "===== Finished " << name << " test" << std::endl;

#define TEST_SUBTEST( n, name ) \
    std::cout << "=== TEST " << n << ": Checking " << name << " ..."<< std::endl;


#endif /* STATS_RRDB_TESTS_H_ */
