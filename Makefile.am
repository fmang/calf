CLEANFILES = htmlsnips.h

bin_PROGRAMS = calf
calf_SOURCES = calf.c calf.h html.c snips.html
calf_CFLAGS = $(SYSTEMD_DAEMON_CFLAGS)
calf_LDADD = $(SYSTEMD_DAEMON_LIBS)

htmlsnips.h: snips.html mksnips.sed
	sed -f mksnips.sed $< > $@
