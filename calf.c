#include "calf.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#ifdef USE_TIMERS
#  include <sys/time.h>
#endif

struct context ctx;

/*******************************************************************************
 * Calendars
 */

static int is_visible(const struct dirent *entry)
{
	return entry->d_name[0] != '.';
}

static struct calendar *scan()
{
	struct dirent **entries;
	int entry_count = scandir(".", &entries, is_visible, alphasort);
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

static int list(struct tm *date, struct entry ***entries)
{
	char dirpath[128];
	strftime(dirpath, 128, "%F", &ctx.date);
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
		asprintf(&entry->path, "%s/%s", dirpath, items[i]->d_name);
		entry->name = entry->path + strlen(dirpath) + 1;
		stat(entry->path, &entry->st);
		(*entries)[i] = entry;
	}
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

static int set_current_date()
{
	const char *uri = getenv("DOCUMENT_URI");
	if (strncmp(ctx.base_uri, uri, strlen(ctx.base_uri)) != 0)
		return 1;
	uri += strlen(ctx.base_uri);
	char *pos;
	if ((pos = strptime(uri, "/%F", &ctx.date))) {
		if (*pos == '\0' || strcmp(pos, "/") == 0)
			return 0;
	} else if (*uri == '\0' || strcmp(uri, "/") == 0) {
		time_t now;
		time(&now);
		memcpy(&ctx.date, gmtime(&now), sizeof(struct tm));
		return 0;
	}
	return 1;
}

static int init_context()
{
	const char *doc_root = getenv("CALF_ROOT");
	if (!doc_root)
		doc_root = getenv("DOCUMENT_ROOT");
	if (!doc_root) {
		fputs("No CALF_ROOT or DOCUMENT_ROOT set.\n", stderr);
		return -1;
	}
	chdir(doc_root);
	ctx.base_uri = getenv("CALF_URI");
	if (!ctx.base_uri)
		ctx.base_uri = "";
	ctx.title = getenv("CALF_TITLE");
	if (!ctx.title)
		ctx.title = "Calf";
	return set_current_date();
}

/*******************************************************************************
 * Main
 */

static void page()
{
	struct entry **entries;
	ctx.calendars = scan();
	list(&ctx.date, &entries);

	html_header(&ctx);
	html_calendars(&ctx);
	html_listing(&ctx, entries);
	html_footer();

	free_calendars(ctx.calendars);
	free_entries(entries);
}

static int process()
{
#ifdef USE_TIMERS
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
#endif
	int rc = init_context();
	if (rc == -1)
		return EXIT_FAILURE;
	if (rc == 1) {
		puts(
		    "Content-Type: text/html\n"
		    "Status: 404 Not Found\n"
		);
		html_404();
		return EXIT_SUCCESS;
	}
	puts(
	    "Content-Type: text/html\n"
	    "Status: 200 OK\n"
	);
	page();
#ifdef USE_TIMERS
	gettimeofday(&end, NULL);
	fprintf(stderr, "%s generated in %ld microseconds\n",
		getenv("DOCUMENT_URI"),
		(end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec)
	);
#endif
	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
#ifdef HAVE_FCGI
#ifdef HAVE_SYSTEMD
	if (sd_listen_fds(0) >= 1)
		dup2(SD_LISTEN_FDS_START, 0);
#endif
	while (!FCGI_Accept()) {
		FCGI_SetExitStatus(process());
		FCGI_Finish();
	}
	return EXIT_SUCCESS;
#else
	return process();
#endif
}
