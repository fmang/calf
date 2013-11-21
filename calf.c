#include "calf.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

struct tm current_date;
const char *base_uri;

void free_cal(struct cal_t *cur)
{
	struct cal_t *next;
	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
}

int set_current_date()
{
	const char *uri = getenv("DOCUMENT_URI");
	if (strncmp(base_uri, uri, strlen(base_uri)) != 0)
		return 0;
	uri += strlen(base_uri);
	char *pos;
	if ((pos = strptime(uri, "/%F", &current_date))) {
		if (*pos == '\0' || strcmp(pos, "/") == 0)
			return 1;
	} else if (*uri == '\0' || strcmp(uri, "/") == 0) {
		time_t now;
		time(&now);
		memcpy(&current_date, gmtime(&now), sizeof(struct tm));
		return 1;
	}
	return 0;
}

int is_visible(const struct dirent *entry)
{
	return entry->d_name[0] != '.';
}

struct calendar {
	int year;
	uint32_t months[12];
	struct calendar *next;
};

struct calendar *scan()
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

void display_calendars(struct calendar *cal)
{
	puts("<div id=\"calendars\">");
	for (; cal; cal = cal->next) {
		int current_year = cal->year == current_date.tm_year;
		for (int i = 0; i < 12; i++) {
			int day = 0;
			if (current_year && i == current_date.tm_mon)
				day = current_date.tm_mday;
			if (cal->months[i] == 0)
				continue;
			html_cal(cal->year, i, day, cal->months[i]);
		}
	}
	puts("</div>");
}

void free_calendars(struct calendar *cal)
{
	struct calendar *next;
	while (cal) {
		next = cal->next;
		free(cal);
		cal = next;
	}
}

int process()
{
	const char *doc_root = getenv("CALF_ROOT");
	if (!doc_root) {
		doc_root = getenv("DOCUMENT_ROOT");
		if (!doc_root) {
			fputs("No CALF_ROOT or DOCUMENT_ROOT set.\n", stderr);
			return EXIT_FAILURE;
		}
	}
	chdir(doc_root);

	base_uri = getenv("CALF_URI");
	if (!base_uri) base_uri = "";
	if (!set_current_date()) {
		puts(
		    "Content-Type: text/html\n"
		    "Status: 404 Not Found\n"
		    "\n"
		);
		html_404();
		return EXIT_SUCCESS;
	}

	char *title = getenv("CALF_TITLE");
	if (!title) title = "Calf";
	char buf[128];
	strftime(buf, 128, "%B %-d, %Y", &current_date);
	puts(
	    "Content-Type: text/html\n"
	    "Status: 200 OK\n"
	    "\n"
	);
	html_header(title, base_uri, buf);

	struct calendar *cal = scan();
	display_calendars(cal);
	free_calendars(cal);

	puts("<div id=\"listing\">");
	printf("<h2>%s</h2>", buf);
	strftime(buf, 128, "%F", &current_date);
	struct dirent **entries;
	int entry_count = scandir(buf, &entries, is_visible, alphasort);
	if (entry_count > 0) {
		char *path = 0;
		struct stat st;
		puts("<ul>");
		for (int i = 0; i < entry_count; i++) {
			puts("<li>");
			path = (char*) realloc(path, strlen(entries[i]->d_name) + 12);
			strncpy(path, buf, 10);
			path[10] = '/';
			strcpy(path+11, entries[i]->d_name);
			printf("<a href=\"%s/%s/", base_uri, buf);
			html_escape(entries[i]->d_name);
			printf("\">");
			if (stat(path, &st) == 0) {
				puts("<span class=\"size\">");
				if (S_ISDIR(st.st_mode))
					fputs("[DIR]", stdout);
				else if (st.st_size < 1024)
					printf("%lu B", st.st_size);
				else if (st.st_size < 1024*1024)
					printf("%lu KB", st.st_size/1024);
				else if (st.st_size < 1024*1024*1024)
					printf("%lu MB", st.st_size/(1024*1024));
				else
					printf("%lu GB", st.st_size/(1024*1024*1024));
				puts("</span>");
			}
			html_escape(entries[i]->d_name);
			puts("</a></li>");
			free(entries[i]);
		}
		puts("</ul>");
		if (path) free(path);
		free(entries);
	} else if (entry_count < 0) {
		puts("<span>Nothing.</span>");
	} else if (entry_count <= 0) {
		puts("<span>Well&hellip;</span>");
	}
	puts("</div><div style=\"clear:both;\"></div>");

	html_footer();

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
