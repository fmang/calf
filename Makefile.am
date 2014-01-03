CLEANFILES = htmlsnips.h

bin_PROGRAMS = calf
calf_SOURCES = calf.c calf.h
calf_CFLAGS = $(SYSTEMD_DAEMON_CFLAGS) $(URIPARSER_CFLAGS)
calf_LDADD = $(SYSTEMD_DAEMON_LIBS) $(URIPARSER_LIBS)

htmlsnips.h: snips.html mksnips.sed
	sed -f mksnips.sed $< > $@
