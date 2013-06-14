NULL=

bin_PROGRAMS= \
	stats-rrdb \
	$(NULL)
	
stats_rrdb_SOURCES= \
	config.cpp \
	exception.cpp \
	log.cpp \
	parser/grammar.cpp \
	parser/interval.cpp \
	parser/retention_policy.cpp \
	parser/statements.cpp \
	rrdb/rrdb.cpp \
	rrdb/rrdb_metric.cpp \
	rrdb/rrdb_metric_block.cpp \
	rrdb/rrdb_metric_tuple.cpp \
	server_tcp.cpp \
	server_udp.cpp \
	server.cpp \
	stats_rrdb.cpp \
	thread_pool.cpp \
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




