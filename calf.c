#include "calf.h"

#ifdef USE_TIMERS
#  include <sys/time.h>
#endif

/*******************************************************************************
 * Context
 */

static int scan_uri(const char *uri, struct tm *date)
{
	if (!strcmp(uri, "/")) {
		time_t now;
		time(&now);
		memcpy(date, gmtime(&now), sizeof(struct tm));
		return 0;
	}
	char *pos = strptime(uri, "/%Y/%m", date);
	if (pos && (*pos == '\0' || !strcmp(pos, "/")))
		return 0;
	return -1;
}

static char *canonical_uri(struct tm *date)
{
	char *uri = malloc(16);
	strftime(uri, 16, "/%Y/%m/", date);
	return uri;
}

static int init_context(struct context *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	const char *root = getenv("DOCUMENT_ROOT");
	if (!root) {
		fputs("No DOCUMENT_ROOT set.\n", stderr);
		return -1;
	}
	ctx->uri = getenv("DOCUMENT_URI");
	if (!ctx->uri) {
		fputs("No DOCUMENT_URI set.\n", stderr);
		return -1;
	}
	ctx->title = getenv("CALF_TITLE");
	if (!ctx->title)
		ctx->title = "Calf";
	return 0;
}

static void free_context(struct context *ctx)
{
}

/*******************************************************************************
 * Main
 */

void do_things(struct context *ctx)
{
	puts("Content-Type: text/plain\n");
	if (scan_uri(ctx->uri, &ctx->date) == -1) {
		puts("404 Not Found");
	} else {
		char *canon = canonical_uri(&ctx->date);
		puts(canon);
		free(canon);
	}
}

#ifdef USE_TIMERS
static void debug_time(char *label, struct timeval *begin, struct timeval *end)
{
	fprintf(stderr, "%s %s time: %ld microseconds\n",
		getenv("DOCUMENT_URI"), label,
		(end->tv_sec - begin->tv_sec) * 1000000 + (end->tv_usec - begin->tv_usec)
	);
}
#endif

static int process()
{
	struct context ctx;
#ifdef USE_TIMERS
	struct timeval begin, mid, end;
	gettimeofday(&begin, NULL);
#endif
	if (init_context(&ctx))
		return EXIT_FAILURE;
#ifdef USE_TIMERS
	gettimeofday(&mid, NULL);
	debug_time("context generation", &begin, &mid);
#endif
	do_things(&ctx);
#ifdef USE_TIMERS
	gettimeofday(&end, NULL);
	debug_time("HTML generation", &mid, &end);
	debug_time("total", &begin, &end);
#endif
	free_context(&ctx);
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
#ifdef HAVE_LIBFCGI
#  ifdef HAVE_SYSTEMD
	if (sd_listen_fds(0) >= 1)
		dup2(SD_LISTEN_FDS_START, 0);
#  endif
	while (!FCGI_Accept()) {
		FCGI_SetExitStatus(process());
		FCGI_Finish();
	}
	return EXIT_SUCCESS;
#else
	return process();
#endif
}
