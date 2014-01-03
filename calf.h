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
	const char *uri;
	struct entry **entries;
};

int is_visible(const struct dirent *entry);
int list(const char *root, struct tm *date, struct entry ***entries);
void free_entries(struct entry **entries);

void html_main(struct context *ctx);
