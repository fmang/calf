#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE /* asprintf, strptime */

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifdef HAVE_LIBFCGI
#  include <fcgi_stdio.h>
#  ifdef HAVE_SYSTEMD
#    include <systemd/sd-daemon.h>
#  endif
#endif

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

struct entry {
	ino_t ino;
	char *path;
	char *name;
	struct stat st;
};

void html_escape(const char *str);
void html_calendars(struct context *ctx);
void html_404();
void html_header(struct context *ctx);
void html_footer();
