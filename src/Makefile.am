pkglibexec_PROGRAMS = calf.cgi
calf_cgi_SOURCES = calf.c calf.h html.c snips.html
calf_cgi_CFLAGS = -D_XOPEN_SOURCE -D_GNU_SOURCE $(SYSTEMD_DAEMON_CFLAGS) $(URIPARSER_CFLAGS)
calf_cgi_LDADD = $(SYSTEMD_DAEMON_LIBS) $(URIPARSER_LIBS)

nodist_calf_cgi_SOURCES = htmlsnips.h
BUILT_SOURCES = htmlsnips.h
CLEANFILES = htmlsnips.h

dist_noinst_SCRIPTS = mksnips.sed

htmlsnips.h: snips.html mksnips.sed
	sed -f mksnips.sed $< > $@

if FASTCGI
install-exec-hook:
	cd $(DESTDIR)$(pkglibexecdir) && \
	$(LN_S) -f calf.cgi calf.fcgi
endif
