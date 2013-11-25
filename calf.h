#ifdef HAVE_CONFIG_H
#  include "config.h"
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

struct entry {
	ino_t ino;
	char *path;
	struct tm *date;
	char *name;
	struct stat st;
};

struct context {
	struct tm date;
	const char *base_uri;
	const char *title;
	struct calendar *calendars;
	struct entry **entries;
};

void html_main(struct context *ctx);
