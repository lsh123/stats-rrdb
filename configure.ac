dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT(stats-rrdb, 0.9.8)

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

dnl Setup pedantic compilation
CFLAGS="$CFLAGS -pedantic -Wall -Werror -Wformat=2 -Wno-variadic-macros -fstack-protector --param=ssp-buffer-size=4 -fexceptions"
CPPFLAGS="$CPPFLAGS -pedantic -Wall -Werror -Wformat=2 -Wno-variadic-macros -fstack-protector --param=ssp-buffer-size=4 -fexceptions"

dnl Add grpof option
AC_ARG_ENABLE(gprof,
            AS_HELP_STRING([--enable-gprof=@<:@no/yes@:>@],
            [build with support for gprof]),,
            [enable_gprof=no])
if test "x$enable_gprof" != "xno" ; then
    CFLAGS="$CFLAGS -pg"
    CPPFLAGS="$CPPFLAGS -pg"
    LDFLAGS="$LDFLAGS -pg"
fi

dnl hard code the user/group for now
AC_SUBST(STATS_RRDB_USER,  "statsrrdb")
AC_SUBST(STATS_RRDB_GROUP, "statsrrdb")

AC_CONFIG_FILES(
	Makefile 
	deploy/Makefile
	src/Makefile
	stats-rrdb.spec
)

AC_OUTPUT

