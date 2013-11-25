#include "calf.h"
#include "htmlsnips.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
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

/*******************************************************************************
 * Date-related functions
 * Calendar generation
 */

static int is_leap_year(struct tm *date)
{
	if (date->tm_year % 400 == 0)
		return 1;
	if (date->tm_year % 100 == 0)
		return 0;
	if (date->tm_year % 4 == 0)
		return 1;
	return 0;
}

static int month_length(struct tm *date)
{
	int month = date->tm_mon + 1;
	if (month == 2)
		return is_leap_year(date) ? 29 : 28;
	if (month == 4 || month == 6 || month == 9 || month == 11)
		return 30;
	return 31;
}

static void format_calendar(struct context *ctx, struct calendar *cal, int month)
{
	struct tm date = {
		.tm_mday = 1,
		.tm_mon  = month,
		.tm_year = cal->year,
	};
	if (mktime(&date) == -1)
		return;
	int current_year = cal->year == ctx->date.tm_year;
	int day = 0;
	if (current_year && month == ctx->date.tm_mon)
		day = ctx->date.tm_mday;
	int last_day = month_length(&date);
	printf(snip_calendar_header, ft(snip_date_calendar_title, &date));
	while (date.tm_mday <= last_day) {
		if (date.tm_wday == 1 || date.tm_mday == 1)
			printf(snip_calendar_week);
		if (date.tm_mday == 1) { /* pad */
			int dow = (date.tm_wday + 6) % 7;
			for (int i = 0; i < dow; i++)
				put(snip_calendar_empty_cell);
		}
		int today = date.tm_mday == day;
		int linked = cal->months[month] & (1 << (date.tm_mday - 1));
		printf(
			today  ? snip_calendar_current_day :
			linked ? snip_calendar_linked_day  :
			snip_calendar_regular_day,
			ft(snip_date_calendar_day, &date),
			ctx->base_uri, ft("%F", &date)
		);
		if (date.tm_wday == 0)
			printf(snip_calendar_week_end);
		/* increment */
		date.tm_mday++;
		date.tm_wday = (date.tm_wday + 1) % 7;
		date.tm_yday++;
	}
	if (date.tm_wday != 0) { /* pad */
		int dow = date.tm_wday - 1;
		for (int i = 0; i < 6 - dow; i++)
			put(snip_calendar_empty_cell);
		printf(snip_calendar_week_end);
	}
	printf(snip_calendar_footer);
}

static void html_calendars(struct context *ctx)
{
	put(snip_calendars_header);
	for (struct calendar *cal = ctx->calendars; cal; cal = cal->next) {
		for (int i = 0; i < 12; i++) {
			if (cal->months[i] == 0)
				continue;
			format_calendar(ctx, cal, i);
		}
	}
	put(snip_calendars_footer);
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
	struct entry **entries = ctx->entries;
	printf(snip_listing_header, ft(snip_date_listing_header, &ctx->date));
	if (!entries) {
		put(snip_listing_empty);
		goto end;
	}
	for (; *entries; entries++) {
		printf(snip_listing_entry,
			ctx->base_uri,
			ft("%F", (*entries)->date),
			uri_escape((*entries)->name),
			format_size(&(*entries)->st),
			entities((*entries)->name)
		);
	}
end:
	put(snip_listing_footer);
}

/*******************************************************************************
 * Pages
 */

static void html_404()
{
	put(
	    "Content-Type: text/html\n"
	    "Status: 404 Not Found\n"
	    "\n"
	);
	put(snip_404);
}

static void html_200(struct context *ctx)
{
	obstack_init(&ob);
	put(
	    "Content-Type: text/html\n"
	    "Status: 200 OK\n"
	    "\n"
	);
	printf(snip_header, ft(snip_date_title, &ctx->date), ctx->title, ctx->base_uri);
	html_calendars(ctx);
	html_listing(ctx);
	put(snip_footer);
	obstack_free(&ob, NULL);
}

void html_main(struct context *ctx)
{
	if (ctx->calendars)
		html_200(ctx);
	else
		html_404();
}
