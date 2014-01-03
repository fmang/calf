#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define _GNU_SOURCE /* asprintf, strptime */

#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifdef HAVE_LIBFCGI
#  include <fcgi_stdio.h>
#  ifdef HAVE_SYSTEMD
#    include <systemd/sd-daemon.h>
#  endif
#endif

struct context {
	struct tm date;
	const char *title;
	const char *uri;
};

void html_main(struct context *ctx);
