#include "calf.h"
#include "htmlsnips.h"
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

static char *entities(const char *str)
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

static char *format_size(struct stat *st)
{
	if (S_ISDIR(st->st_mode))
		return "[DIR]";
	static const char *units[] = { "B", "KiB", "MiB", "GiB" };
	size_t size = st->st_size;
	int i;
	for (i = 0; i < 4 && size >= 1024; i++)
		size /= 1024;
	obstack_printf(&ob, "%lu %s", size, units[i]);
	obstack_1grow(&ob, '\0');
	return obstack_finish(&ob);
}

static void html_listing(struct context *ctx)
{
	void **entries = NULL;
	printf(snip_listing_header, ft(snip_date_listing_header, &ctx->date));
	if (!entries) {
		put(snip_listing_empty);
		goto end;
	}
	for (; *entries; entries++) {
	}
end:
	put(snip_listing_footer);
}

/*******************************************************************************
 * Pages
 */

void html_main(struct context *ctx)
{
	obstack_init(&ob);
	printf(snip_header, ft(snip_date_title, &ctx->date), ctx->title);
	html_listing(ctx);
	put(snip_footer);
	obstack_free(&ob, NULL);
}
