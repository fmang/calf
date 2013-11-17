CLEANFILES = snips.h

bin_PROGRAMS = calf
calf_SOURCES = calf.c calf.h html.c snips.h
calf_CFLAGS = $(SYSTEMD_DAEMON_CFLAGS)
calf_LDADD = $(SYSTEMD_DAEMON_LIBS)

snips.h: snips mksnips.sed
	sed -f mksnips.sed $< > $@
