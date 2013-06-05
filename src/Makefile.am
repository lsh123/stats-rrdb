NULL=

bin_PROGRAMS= \
	statsrrdb \
	$(NULL)
	
statsrrdb_SOURCES= \
	log.cpp \
	exception.cpp \
	config.cpp \
	rrdb.cpp \
	server.cpp \
	stats-rrdb.cpp \
	$(NULL)

statsrrdb_CPPFLAGS= \
	$(BOOST_CPPFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_CPPFLAGS) \
	$(BOOST_SYSTEM_CPPFLAGS) \
	$(BOOST_THREADS_CPPFLAGS) \
	$(NULL)

statsrrdb_LDFLAGS= \
	$(BOOST_LDPFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_LDPFLAGS) \
	$(BOOST_SYSTEM_LDFLAGS) \
	$(BOOST_THREADS_LDFLAGS) \
	$(NULL)

statsrrdb_LDADD= \
	$(BOOST_LIBS) \
	$(BOOST_PROGRAM_OPTIONS_LIBS) \
	$(BOOST_SYSTEM_LIBS) \
	$(BOOST_THREADS_LIBS) \
	$(NULL)




