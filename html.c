#include "calf.h"
#include "htmlsnips.h"

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <uriparser/Uri.h>

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

/*******************************************************************************
 * Listing
 */

int is_visible(const struct dirent *entry)
{
	return entry->d_name[0] != '.';
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
	struct dirent **items;
	int count = scandir(dirpath, &items, is_visible, alphasort);
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
	if (count >= 0)
		free(items);
	put(snip_listing_table_footer);
	return 0;
}

static void listing(char *dirpath, char *name)
{
	printf(snip_listing_header, name);
	list_files(dirpath, name);
	put(snip_listing_footer);
}

static int html_listings(struct context *ctx)
{
	struct dirent **items;
	char *dirpath = concat(ctx->root, ft("/%Y/%m/", &ctx->date));
	int count = scandir(dirpath, &items, is_visible, alphasort);
	for (int i = 0; i < count; i++) {
		char *path = concat(concat(dirpath, items[i]->d_name), "/");
		listing(path, items[i]->d_name);
		free(items[i]);
	}
	if (count >= 0)
		free(items);
	return count;
}

/******************************************************************************/

void html_main(struct context *ctx)
{
	obstack_init(&ob);
	printf(snip_header, ft(snip_date_title, &ctx->date), ctx->title);
	html_listings(ctx);
	put(snip_footer);
	obstack_free(&ob, NULL);
}
