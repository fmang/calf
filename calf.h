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

#define __USE_XOPEN /* for strptime */
#include <time.h>

struct calendar {
	int year;
	uint32_t months[12];
	struct calendar *next;
};

struct context {
	struct tm date;
	const char *base_uri;
	const char *title;
	struct calendar *calendars;
};

void html_escape(const char *str);
void html_calendars(struct context *ctx);
void html_404();
void html_header(struct context *ctx);
void html_footer();
