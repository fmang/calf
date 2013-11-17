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

#define __USE_XOPEN
#include <time.h>

struct cal_t {
	struct tm date;
	struct cal_t *next;
};

extern struct tm current_date;
extern const char *base_uri;

void html_escape(const char *str);
struct cal_t* html_calendar(struct cal_t *cal);
void html_404();
void html_header(const char *title, const char *base_uri, const char *date);
void html_footer();
