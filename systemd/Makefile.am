noinst_DATA = calf.service
CLEANFILES = calf.service
EXTRA_DIST = calf.service.in calf.socket

calf.service: calf.service.in Makefile
	sed -e 's,[@]pkglibexecdir[@],$(pkglibexecdir),g' < $< > $@
