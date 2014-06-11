#include "calf.h"
#include "htmlsnips.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uriparser/Uri.h>

#ifdef HAVE_LIBFCGI
#  include <stdarg.h>
#endif

#include <obstack.h>
#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

static struct obstack ob;

/*******************************************************************************
 * Input/Output
 * Formatting
 */

static int put(const char *str)
{
	return fputs(str, stdout);
}

#ifdef HAVE_LIBFCGI
#undef printf
#define printf printf_that_should_not_exist
static int printf_that_should_not_exist(const char *format, ...)
{
	/* FastCGI uses a homemade substitute for printf. */
	/* Unfortunately, calf isn't buying it, sooo... */
	char *result;
	va_list ap;
	va_start(ap, format);
	int rc = obstack_vprintf(&ob, format, ap);
	va_end(ap);
	obstack_1grow(&ob, '\0');
	result = obstack_finish(&ob);
	put(result);
	obstack_free(&ob, result);
	return rc;
}
#endif

static char *ft(const char *format, struct tm *date)
{
	size_t len = strftime(NULL, -1, format, date) + 1;
	char *out = obstack_alloc(&ob, len);
	strftime(out, len, format, date);
	return out;
}

static char *html_escape(const char *str)
{
	for (; *str; str++) {
		const char *entity =
			*str == '"' ? "&quot;" :
			*str == '&' ? "&amp;" :
			*str == '<' ? "&lt;" :
			*str == '>' ? "&gt;" : NULL;
		if (entity)
			obstack_grow(&ob, entity, strlen(entity));
		else
			obstack_1grow(&ob, *str);
	}
	obstack_1grow(&ob, '\0');
	return obstack_finish(&ob);
}

static char *uri_escape(const char *uri)
{
	char *out = obstack_alloc(&ob, 3 * strlen(uri) + 1);
	uriEscapeA(uri, out, URI_FALSE, URI_FALSE);
	return out;
}

static char *concat(const char *s1, const char *s2)
{
	size_t l1 = strlen(s1);
	size_t l2 = strlen(s2);
	char *out = obstack_alloc(&ob, l1 + l2 + 1);
	memcpy(out, s1, l1);
	memcpy(out + l1, s2, l2);
	out[l1 + l2] = '\0';
	return out;
}

/******************************************************************************/

static int is_visible(const struct dirent *entry)
{
	return entry->d_name[0] != '.';
}

static char *drop_extension(const char *filename)
{
	char *dot = strrchr(filename, '.');
	if (!dot)
		return NULL;
	size_t base_len = dot - filename + 1;
	char *base = obstack_alloc(&ob, base_len);
	strncpy(base, filename, base_len - 1);
	base[base_len - 1] = '\0';
	return base;
}

static int is_regular_file(const char *path)
{
	struct stat st;
	memset(&st, 0, sizeof(st));
	stat(path, &st);
	return S_ISREG(st.st_mode);
}

static int list_thumbs(char *dirpath, char *name)
{
	char *thumbsdir = concat(dirpath, ".thumbs/");
	struct dirent **items = NULL;
	int count = scandir(thumbsdir, &items, is_visible, alphasort);
	if (count < 0)
		return -1;
	if (count == 0)
		return 0;
	put(snip_listing_thumbnails_header);
	for (int i = 0; i < count; free(items[i]), i++) {
		char *base = drop_extension(items[i]->d_name);
		if (!base || access(concat(dirpath, base), F_OK))
			continue; /* thumbnail to non-existent file */
		if (!is_regular_file(concat(thumbsdir, items[i]->d_name)))
			continue;
		printf(snip_listing_thumbnail,
			uri_escape(name), uri_escape(items[i]->d_name),
			uri_escape(base), html_escape(base)
		);
	}
	if (count > 0)
		free(items);
	put(snip_listing_thumbnails_footer);
	return 0;
}

/******************************************************************************/

static int to_list(const struct dirent *entry)
{
	return is_visible(entry) && strcmp(entry->d_name, "thumbs");
}

static char *format_size(struct stat *st)
{
	if (S_ISDIR(st->st_mode))
		return "[DIR]";
	static const char *units[] = { "B", "KiB", "MiB", "GiB" };
	size_t size = st->st_size;
	int i;
	for (i = 0; i < 4 && size >= 1024; i++)
		size /= 1024;
	obstack_printf(&ob, "%zu %s", size, units[i]);
	obstack_1grow(&ob, '\0');
	return obstack_finish(&ob);
}

static int list_files(char *dirpath, char *name)
{
	struct dirent **items = NULL;
	int count = scandir(dirpath, &items, to_list, alphasort);
	if (count < 0)
		return -1;
	put(snip_listing_table_header);
	if (count == 0)
		put(snip_listing_table_empty);
	for (int i = 0; i < count; i++) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		stat(concat(dirpath, items[i]->d_name), &st);
		printf(snip_listing_table_entry,
			uri_escape(name), uri_escape(items[i]->d_name),
			format_size(&st), html_escape(items[i]->d_name)
		);
		free(items[i]);
	}
	if (count > 0)
		free(items);
	put(snip_listing_table_footer);
	return 0;
}

/******************************************************************************/

static void listing(char *dirpath, char *name)
{
	printf(snip_listing_header, html_escape(name));
	list_thumbs(dirpath, name);
	list_files(dirpath, name);
	put(snip_listing_footer);
}

static int listings(struct context *ctx)
{
	struct dirent **items = NULL;
	char *dirpath = concat(ctx->root, ft("/%Y/%m/", &ctx->date));
	int count = scandir(dirpath, &items, is_visible, alphasort);
	for (int i = 0; i < count; i++) {
		char *path = concat(concat(dirpath, items[i]->d_name), "/");
		listing(path, items[i]->d_name);
		free(items[i]);
	}
	if (count > 0)
		free(items);
	return count;
}

/******************************************************************************/

static int is_min_number(const struct dirent *entry)
{
	if (*entry->d_name == '0')
		return 0;
	for (const char *c = entry->d_name; *c; c++) {
		if (!isdigit(*c))
			return 0;
	}
	return 1;
}

static void list_months(struct context *ctx, int year)
{
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = year - 1900;
	for (tm.tm_mon = 0; tm.tm_mon <= 11; tm.tm_mon++) {
		if (tm.tm_mon != 0)
			put(snip_month_separator);
		int current = tm.tm_year == ctx->date.tm_year && tm.tm_mon == ctx->date.tm_mon;
		char *path = concat(ctx->root, ft("/%Y/%m", &tm));
		put(ft(
			current ? snip_month_current :
			!access(path, F_OK) ? snip_month_linked :
			snip_month_regular,
			&tm
		));
	}
}

static int scan_years(struct context *ctx, int *previous, int *next)
{
	*previous = *next = -1;
	struct dirent **items = NULL;
	int count = scandir(ctx->root, &items, is_min_number, alphasort);
	if (count < 0)
		return -1;
	char *root = concat(ctx->root, "/");
	int this_year = ctx->date.tm_year + 1900;
	for (int i = 0; i < count; i++) {
		int year = atoi(items[i]->d_name);
		if (year < this_year)
			*previous = year;
		else if (year > this_year && *next == -1)
			*next = year;
		free(items[i]);
	}
	if (count > 0)
		free(items);
	return 0;
}

static void get_last_month(struct context *ctx, int year, struct tm *month)
{
	memset(month, 0, sizeof(*month));
	month->tm_year = year - 1900;
	for (month->tm_mon = 11; month->tm_mon >= 0; month->tm_mon--) {
		char *path = concat(ctx->root, ft("/%Y/%m", month));
		if (!access(path, F_OK))
			return;
	}
	month->tm_mon = 11;
}

static void get_first_month(struct context *ctx, int year, struct tm *month)
{
	memset(month, 0, sizeof(*month));
	month->tm_year = year - 1900;
	for (month->tm_mon = 0; month->tm_mon <= 11; month->tm_mon++) {
		char *path = concat(ctx->root, ft("/%Y/%m", month));
		if (!access(path, F_OK))
			return;
	}
	month->tm_mon = 0;
}

static int list_years(struct context *ctx)
{
	int previous, next;
	if (scan_years(ctx, &previous, &next) < 0)
		return -1;
	struct tm month;
	put(snip_calendar_header);
	if (previous != -1) {
		get_last_month(ctx, previous, &month);
		put(ft(snip_year_previous, &month));
	} else {
		put(snip_year_previous_inactive);
	}
	put(ft(snip_year_current, &ctx->date));
	if (next != -1) {
		get_first_month(ctx, next, &month);
		put(ft(snip_year_next, &month));
	} else {
		put(snip_year_next_inactive);
	}
	put(snip_calendar_neck);
	list_months(ctx, ctx->date.tm_year + 1900);
	put(snip_calendar_footer);
	return 0;
}

/******************************************************************************/

void html_main(struct context *ctx)
{
	obstack_init(&ob);
	char *title = ft(snip_date_title, &ctx->date);
	printf(snip_header, title, html_escape(ctx->title));
	list_years(ctx);
	printf(snip_title, title);
	if (listings(ctx) <= 0)
		put(snip_empty);
	put(snip_footer);
	obstack_free(&ob, NULL);
}
