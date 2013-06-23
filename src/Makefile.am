NULL=

lib_LTLIBRARIES= \
	libstats-rrdb.la \
	$(NULL)
	
bin_PROGRAMS= \
	stats-rrdb \
	stats-rrdb-tests \
	$(NULL)

EXTRA_DIST = \
	common/config.h \
	common/exception.h \
	common/log.h \
	common/memory_buffer.h \
	common/spinlock.h \
	common/lru_cache.h \
	common/types.h \
	parser/grammar.h \
	parser/interval.h \
	parser/interval_grammar.h \
	parser/memory_size.h \
	parser/memory_size_grammar.h \
	parser/retention_policy.h \
	parser/retention_policy_grammar.h \
	parser/statements.h \
	parser/statements_grammar.h \
	rrdb/rrdb.h \
	rrdb/rrdb_files_cache.h \
	rrdb/rrdb_metric.h \
	rrdb/rrdb_metric_block.h \
	rrdb/rrdb_metric_tuples_cache.h \
	rrdb/rrdb_metric_tuple.h \
	server/server.h \
	server/server_tcp.h \
	server/server_udp.h \
	server/thread_pool.h \
	tests/files_cache_tests.h \
	tests/lru_tests.h \
	tests/parsers_tests.h \
	tests/query_tests.h \
	tests/update_tests.h \
	tests/stats_rrdb_tests.h \
	$(NULL)


CUSTOM_CPPFLAGS= \
	$(BOOST_CPPFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_CPPFLAGS) \
	$(BOOST_SYSTEM_CPPFLAGS) \
	$(BOOST_FILESYSTEM_CPPFLAGS) \
	$(BOOST_THREAD_CPPFLAGS) \
	$(NULL)

CUSTOM_LDFLAGS= \
	$(BOOST_LDPFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_LDPFLAGS) \
	$(BOOST_SYSTEM_LDFLAGS) \
	$(BOOST_FILESYSTEM_LDFLAGS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(NULL)

CUSTOM_LDADD= \
	$(BOOST_LIBS) \
	$(BOOST_PROGRAM_OPTIONS_LIBS) \
	$(BOOST_SYSTEM_LIBS) \
	$(BOOST_FILESYSTEM_LIBS) \
	$(BOOST_THREAD_LIBS) \
	$(NULL)

#
# libstats-rrdb
#
libstats_rrdb_la_SOURCES= \
	common/config.cpp \
	common/exception.cpp \
	common/log.cpp \
	parser/grammar.cpp \
	parser/interval.cpp \
	parser/memory_size.cpp \
	parser/retention_policy.cpp \
	parser/statements.cpp \
	rrdb/rrdb.cpp \
	rrdb/rrdb_files_cache.cpp \
	rrdb/rrdb_metric.cpp \
	rrdb/rrdb_metric_block.cpp \
	rrdb/rrdb_metric_tuples_cache.cpp \
	rrdb/rrdb_metric_tuple.cpp \
	server/server.cpp \
	server/server_tcp.cpp \
	server/server_udp.cpp \
	server/thread_pool.cpp \
	$(NULL)
libstats_rrdb_la_CPPFLAGS= \
	$(CUSTOM_CPPFLAGS) \
	$(NULL)
libstats_rrdb_la_LDFLAGS= \
	$(CUSTOM_LDFLAGS) \
	-static \
	$(NULL)
libstats_rrdb_la_LIBADD= \
	$(CUSTOM_LDADD) \
	$(NULL)

#
# stats-rrdb (main program)
#
stats_rrdb_SOURCES= \
	stats_rrdb.cpp \
	$(NULL)
stats_rrdb_CPPFLAGS= \
	$(CUSTOM_CPPFLAGS) \
	$(NULL)
stats_rrdb_LDFLAGS= \
	$(CUSTOM_LDFLAGS) \
	-static \
	$(NULL)
stats_rrdb_LDADD= \
	$(CUSTOM_LDADD) \
	libstats-rrdb.la \
	$(NULL)

#
# stats-rrdb-tests (tests)
#
stats_rrdb_tests_SOURCES= \
	tests/files_cache_tests.cpp \
	tests/lru_tests.cpp \
	tests/parsers_tests.cpp \
	tests/query_tests.cpp \
	tests/update_tests.cpp \
	tests/stats_rrdb_tests.cpp \
	$(NULL)
stats_rrdb_tests_CPPFLAGS= \
	$(CUSTOM_CPPFLAGS) \
	$(NULL)
stats_rrdb_tests_LDFLAGS= \
	$(CUSTOM_LDFLAGS) \
	$(NULL)
stats_rrdb_tests_LDADD= \
	$(CUSTOM_LDADD) \
	libstats-rrdb.la \
	$(NULL)

