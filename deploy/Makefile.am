NULL=

#
# Extra dist
#
EXTRA_DIST = \
	init.d/stats-rrdb.in \
	stats-rrdb.conf.in \
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
# Extra files to clean
#
CLEANFILES = \
	$(noinst_SCRIPTS) \
	$(statsrrdb_conf_DATA) \
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
    $(NULL)

#
# Rules
#
init.d/${PACKAGE_NAME}: init.d/stats-rrdb.in
	$(do_subst) < $(srcdir)/init.d/stats-rrdb.in > "init.d/${PACKAGE_NAME}"
	chmod +x "init.d/${PACKAGE_NAME}"

${PACKAGE_NAME}.conf: stats-rrdb.conf.in
	$(do_subst) < $(srcdir)/stats-rrdb.conf.in > "${PACKAGE_NAME}.conf"
