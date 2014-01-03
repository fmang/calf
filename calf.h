#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <time.h>

#ifdef HAVE_LIBFCGI
#  include <fcgi_stdio.h>
#  ifdef HAVE_SYSTEMD
#    include <systemd/sd-daemon.h>
#  endif
#else
#  include <stdio.h>
#endif

struct context {
	struct tm date;
	const char *title;
	const char *uri;
};

void html_main(struct context *ctx);
