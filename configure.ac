dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT(stats-rrdb, 0.5)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()
AC_CONFIG_MACRO_DIR([m4])
LT_INIT

AC_PROG_CXX
BOOST_REQUIRE([1.5.3])
BOOST_STATIC
BOOST_THREADS([mt])
BOOST_SYSTEM([mt])
BOOST_FILESYSTEM([mt])
BOOST_PROGRAM_OPTIONS([mt])

dnl setup pedantic compilation
CFLAGS="$CFLAGS -pedantic -Wall -Werror -Wformat=2 -Wno-variadic-macros"
CPPFLAGS="$CPPFLAGS -pedantic -Wall -Werror -Wformat=2 -Wno-variadic-macros"

AC_CONFIG_FILES(
	Makefile 
	src/Makefile
	
	stats-rrdb.spec
	stats-rrdb
)

AC_OUTPUT

