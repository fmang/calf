#include "calf.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#ifdef USE_TIMERS
#  include <sys/time.h>
#endif

/*******************************************************************************
 * Calendars
 */

static int is_visible(const struct dirent *entry)
{
	return entry->d_name[0] != '.';
}

static struct calendar *scan(const char *root)
{
	struct dirent **entries;
	int entry_count = scandir(root, &entries, is_visible, alphasort);
	struct tm date;
	char buffer[32];
	struct calendar *cal = NULL;
	struct calendar *new_cal;
	for (int i = entry_count - 1; i >= 0; free(entries[i]), i--) {
		/* ^ reverse processing to end up with an ordered list */
		if (!strptime(entries[i]->d_name, "%F", &date))
			continue;
		/* strptime is too lenient, we want to force canonical ISO 8601. */
		strftime(buffer, 32, "%F", &date);
		if (strcmp(entries[i]->d_name, buffer))
			continue;
		if (!cal || cal->year != date.tm_year) {
			new_cal = calloc(1, sizeof(*new_cal));
			new_cal->year = date.tm_year;
			new_cal->next = cal;
			cal = new_cal;
		}
		cal->months[date.tm_mon] |= 1 << (date.tm_mday - 1);
	}
	free(entries);
	return cal;
}

static void free_calendars(struct calendar *cal)
{
	struct calendar *next;
	while (cal) {
		next = cal->next;
		free(cal);
		cal = next;
	}
}

/*******************************************************************************
 * Listings
 */

static int list(const char *root, struct tm *date, struct entry ***entries)
{
	char dirname[16];
	char *dirpath;
	strftime(dirname, 16, "%F", date);
	asprintf(&dirpath, "%s/%s", root, dirname);
	struct dirent **items;
	int count = scandir(dirpath, &items, is_visible, alphasort);
	if (count > 0)
		*entries = calloc(count + 1, sizeof(**entries));
	else
		*entries = NULL;
	for (int i = 0; i < count; i++) {
		struct entry *entry;
		entry = malloc(sizeof(*entry));
		entry->ino = items[i]->d_ino;
		entry->date = date;
		asprintf(&entry->path, "%s/%s", dirpath, items[i]->d_name);
		entry->name = entry->path + strlen(dirpath) + 1;
		stat(entry->path, &entry->st);
		(*entries)[i] = entry;
		free(items[i]);
	}
	if (count >= 0)
		free(items);
	free(dirpath);
	return count;
}

static void free_entries(struct entry **entries)
{
	if (!entries)
		return;
	for (struct entry **i = entries; *i; i++) {
		free((*i)->path);
		free(*i);
	}
	free(entries);
}

/*******************************************************************************
 * Context
 */

static int scan_uri(const char *uri, const char **base, struct tm *date)
{
	if (!strcmp(uri, "/")) {
		*base = "";
		time_t now;
		time(&now);
		memcpy(date, gmtime(&now), sizeof(struct tm));
		return 0;
	}
	char *pos = strptime(uri, "/%F", date);
	if (!pos)
		return -1;
	else if (*pos == '\0')
		*base = "";
	else if (!strcmp(pos, "/"))
		*base = "../";
	else
		return -1;
	return 0;
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
	if (scan_uri(ctx->uri, &ctx->base_uri, &ctx->date) == 0) {
		ctx->calendars = scan(root);
		list(root, &ctx->date, &ctx->entries);
	}
	ctx->title = getenv("CALF_TITLE");
	if (!ctx->title)
		ctx->title = "Calf";
	return 0;
}

static void free_context(struct context *ctx)
{
	free_calendars(ctx->calendars);
	free_entries(ctx->entries);
}

/*******************************************************************************
 * Main
 */

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
	html_main(&ctx);
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
#ifdef HAVE_FCGI
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
