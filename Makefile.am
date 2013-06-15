NULL=

ACLOCAL_AMFLAGS=-I m4

SUBDIRS = \
	deploy \
	src \
	$(NULL)

EXTRA_DIST = \
	AUTHORS \
	ChangeLog \
	COPYING \
	NEWS \
	README.md \
	stats-rrdb.spec \
	$(NULL)

#
# Extra files to clean
#
CLEANFILES = \
	${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz \
	$(NULL)

#
# Extra rules
#
dist-rpm: dist 
	rpmbuild -ta ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
	
