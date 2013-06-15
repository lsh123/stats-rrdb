NULL=

#
# Extra dist
#
EXTRA_DIST = \
	stats-rrdb.conf.in \
	init.d/stats-rrdb.in \
	logrotate.d/stats-rrdb.in \
	$(NULL)

#
# Extra scripts
#
noinst_SCRIPTS = \
	init.d/${PACKAGE_NAME} \
	$(NULL)

EXTRA_SCRIPTS = \
	$(NULL)

#
# Extra configs
#
statsrrdb_confdir = $(sysconfdir)/${PACKAGE_NAME}
statsrrdb_conf_DATA = \
	${PACKAGE_NAME}.conf \
	$(NULL)
	
#
# logrotate configs
#
statsrrdb_logrotate_confdir = $(sysconfdir)/logrotate.d
statsrrdb_logrotate_conf_DATA = \
	logrotate.d/${PACKAGE_NAME} \
	$(NULL)

#
# Extra files to clean
#
CLEANFILES = \
	$(noinst_SCRIPTS) \
	$(statsrrdb_conf_DATA) \
	$(statsrrdb_logrotate_conf_DATA) \
	$(NULL)


#
# Substitute script
#
do_subst = sed \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
    -e 's,[@]localstatedir[@],$(localstatedir),g' \
    -e 's,[@]PACKAGE_NAME[@],$(PACKAGE),g' \
    -e 's,[@]PACKAGE_VERSION[@],$(VERSION),g' \
    -e 's,[@]STATS_RRDB_USER[@],$(STATS_RRDB_USER),g' \
    -e 's,[@]STATS_RRDB_GROUP[@],$(STATS_RRDB_GROUP),g' \
    $(NULL)

#
# Extra rules
#
${PACKAGE_NAME}.conf: stats-rrdb.conf.in
	$(do_subst) < $(srcdir)/stats-rrdb.conf.in > "${PACKAGE_NAME}.conf"

init.d/${PACKAGE_NAME}: init.d/stats-rrdb.in
	$(do_subst) < $(srcdir)/init.d/stats-rrdb.in > "init.d/${PACKAGE_NAME}"
	chmod +x "init.d/${PACKAGE_NAME}"

logrotate.d/${PACKAGE_NAME}: logrotate.d/stats-rrdb.in
	$(do_subst) < $(srcdir)/logrotate.d/stats-rrdb.in > "logrotate.d/${PACKAGE_NAME}"
