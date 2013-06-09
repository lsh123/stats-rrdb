NULL=

bin_PROGRAMS= \
	statsrrdb \
	$(NULL)
	
statsrrdb_SOURCES= \
	config.cpp \
	exception.cpp \
	log.cpp \
	parser/grammar.cpp \
	parser/interval.cpp \
	parser/retention_policy.cpp \
	parser/statements.cpp \
	rrdb.cpp \
	rrdb_metric.cpp \
	server_tcp.cpp \
	server_udp.cpp \
	server.cpp \
	stats_rrdb.cpp \
	thread_pool.cpp \
	$(NULL)

statsrrdb_CPPFLAGS= \
	$(BOOST_CPPFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_CPPFLAGS) \
	$(BOOST_SYSTEM_CPPFLAGS) \
	$(BOOST_FILESYSTEM_CPPFLAGS) \
	$(BOOST_THREAD_CPPFLAGS) \
	$(NULL)

statsrrdb_LDFLAGS= \
	$(BOOST_LDPFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_LDPFLAGS) \
	$(BOOST_SYSTEM_LDFLAGS) \
	$(BOOST_FILESYSTEM_LDFLAGS) \
	$(BOOST_THREAD_LDFLAGS) \
	$(NULL)

statsrrdb_LDADD= \
	$(BOOST_LIBS) \
	$(BOOST_PROGRAM_OPTIONS_LIBS) \
	$(BOOST_SYSTEM_LIBS) \
	$(BOOST_FILESYSTEM_LIBS) \
	$(BOOST_THREAD_LIBS) \
	$(NULL)




