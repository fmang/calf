#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBFCGI
#  include <fcgi_stdio.h>
#  ifdef HAVE_SYSTEMD
#    include <systemd/sd-daemon.h>
#  endif
#else
#  include <stdio.h>
#endif

#include <stdint.h>

#define __USE_XOPEN
#include <time.h>

struct calendar {
	int year;
	uint32_t months[12];
	struct calendar *next;
};

extern struct tm current_date;
extern const char *base_uri;

void html_escape(const char *str);
void html_calendars(struct calendar *cal);
void html_404();
void html_header(const char *title, const char *base_uri, const char *date);
void html_footer();
