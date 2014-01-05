#include "calf.h"
#include <stdlib.h>
#include <string.h>

#ifdef USE_TIMERS
#  include <sys/time.h>
#endif

#ifdef HAVE_LIBFCGI
#  ifdef HAVE_SYSTEMD
#    include <systemd/sd-daemon.h>
#    include <unistd.h>
#  endif
#endif

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

static int init_context(struct context *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->root = getenv("DOCUMENT_ROOT");
	if (!ctx->root) {
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

static void reply(struct context *ctx)
{
	if (scan_uri(ctx->uri, &ctx->date) == -1) {
		puts("Status: 404 Not Found");
		puts("Content-Type: text/plain\n");
		puts("404 Not Found");
		return;
	}
	char *canon = malloc(16);
	strftime(canon, 16, "/%Y/%m/", &ctx->date);
	if (strcmp(canon, ctx->uri)) {
		puts("Status: 303 See Other");
		fputs("Location: ", stdout);
		puts(canon);
		puts("");
		puts(canon);
	} else {
		puts("Status: 200 OK");
		puts("Content-Type: text/html\n");
		html_main(ctx);
	}
	free(canon);
}

static int process()
{
	struct context ctx;
#ifdef USE_TIMERS
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
#endif
	if (init_context(&ctx))
		return EXIT_FAILURE;
	reply(&ctx);
#ifdef USE_TIMERS
	gettimeofday(&end, NULL);
	unsigned long diff = (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
	fprintf(stderr, "%s generated in %lu microseconds\n", ctx.uri, diff);
#endif
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
