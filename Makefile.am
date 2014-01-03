CLEANFILES = htmlsnips.h

bin_PROGRAMS = calf
calf_SOURCES = calf.c calf.h html.c snips.html
calf_CFLAGS = -D_XOPEN_SOURCE -D_GNU_SOURCE $(SYSTEMD_DAEMON_CFLAGS) $(URIPARSER_CFLAGS)
calf_LDADD = $(SYSTEMD_DAEMON_LIBS) $(URIPARSER_LIBS)

html.o: htmlsnips.h

htmlsnips.h: snips.html mksnips.sed
	sed -f mksnips.sed $< > $@
