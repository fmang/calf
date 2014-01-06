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

static void list_months(struct context *ctx, char *dirpath, char *year)
{
	struct tm tm;
	tm.tm_year = atoi(year) - 1900;
	printf(snip_year_header, year);
	for (tm.tm_mon = 0; tm.tm_mon <= 11; tm.tm_mon++) {
		if (tm.tm_mon != 0)
			put(snip_month_separator);
		char *path = concat(dirpath, ft("%m", &tm));
		int current = tm.tm_year == ctx->date.tm_year && tm.tm_mon == ctx->date.tm_mon;
		char *uri = ft("%Y/%m", &tm);
		printf(
			current ? snip_month_current :
			!access(path, F_OK) ? snip_month_linked :
			snip_month_regular,
			ft(snip_date_month, &tm),
			uri
		);
	}
	put(snip_year_footer);
}

static int list_years(struct context *ctx)
{
	struct dirent **items = NULL;
	int count = scandir(ctx->root, &items, is_min_number, alphasort);
	if (count < 0)
		return -1;
	char *root = concat(ctx->root, "/");
	put(snip_calendar_header);
	for (int i = 0; i < count; i++) {
		char *path = concat(concat(root, items[i]->d_name), "/");
		list_months(ctx, path, items[i]->d_name);
		free(items[i]);
	}
	if (count > 0)
		free(items);
	put(snip_calendar_footer);
	return count;
}

/******************************************************************************/

void html_main(struct context *ctx)
{
	obstack_init(&ob);
	printf(snip_header, ft(snip_date_title, &ctx->date), html_escape(ctx->title));
	list_years(ctx);
	if (listings(ctx) <= 0)
		put(snip_empty);
	put(snip_footer);
	obstack_free(&ob, NULL);
}
