if SYSTEMD
MAYBE_SYSTEMD = systemd
endif

SUBDIRS = src share $(MAYBE_SYSTEMD)

EXTRA_DIST = README calf.8.in
CLEANFILES = calf.8

man_MANS = calf.8

calf.8: calf.8.in Makefile
	sed -e 's,[@]pkglibexecdir[@],$(pkglibexecdir),g' \
	    -e 's,[@]pkgdatadir[@],$(pkgdatadir),g' \
	    < $< > $@
