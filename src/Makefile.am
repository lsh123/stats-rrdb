NULL=

bin_PROGRAMS= \
	stats-rrdb \
	$(NULL)

EXTRA_DIST = \
	config.h \
	exception.h \
	log.h \
	memory_buffer.h \
	spinlock.h \
	lru_cache.h \
	types.h \
	parser/grammar.h \
	parser/interval.h \
	parser/interval_grammar.h \
	parser/retention_policy.h \
	parser/retention_policy_grammar.h \
	parser/statements.h \
	parser/statements_grammar.h \
	rrdb/rrdb.h \
	rrdb/rrdb_file_cache.h \
	rrdb/rrdb_metric.h \
	rrdb/rrdb_metric_block.h \
	rrdb/rrdb_metric_tuple.h \
	rrdb/rrdb_test.h \
	server/server.h \
	server/server_tcp.h \
	server/server_udp.h \
	server/thread_pool.h \
	$(NULL)
	
stats_rrdb_SOURCES= \
	config.cpp \
	exception.cpp \
	log.cpp \
	parser/grammar.cpp \
	parser/interval.cpp \
	parser/retention_policy.cpp \
	parser/statements.cpp \
	stats_rrdb.cpp \
	rrdb/rrdb.cpp \
	rrdb/rrdb_file_cache.cpp \
	rrdb/rrdb_metric.cpp \
	rrdb/rrdb_metric_block.cpp \
	rrdb/rrdb_metric_tuple.cpp \
	rrdb/rrdb_test.cpp \
	server/server.cpp \
	server/server_tcp.cpp \
	server/server_udp.cpp \
	server/thread_pool.cpp \
	$(NULL)

stats_rrdb_CPPFLAGS= \
	$(BOOST_CPPFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_CPPFLAGS) \
	$(BOOST_SYSTEM_CPPFLAGS) \
	$(BOOST_FILESYSTEM_CPPFLAGS) \
	$(BOOST_THREAD_CPPFLAGS) \
	$(NULL)

stats_rrdb_LDFLAGS= \
	$(BOOST_LDPFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_LDPFLAGS) \
	$(BOOST_SYSTEM_LDFLAGS) \
	$(BOOST_FILESYSTEM_LDFLAGS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(NULL)

stats_rrdb_LDADD= \
	$(BOOST_LIBS) \
	$(BOOST_PROGRAM_OPTIONS_LIBS) \
	$(BOOST_SYSTEM_LIBS) \
	$(BOOST_FILESYSTEM_LIBS) \
	$(BOOST_THREAD_LIBS) \
	$(NULL)




