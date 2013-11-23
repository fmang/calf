#include "calf.h"
#include "htmlsnips.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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

static int printft(const char *format, struct tm *date, ...)
{
	va_list ap;
	va_start(ap, date);
	obstack_vprintf(&ob, format, ap);
	va_end(ap);
	obstack_1grow(&ob, '\0');
	/* predict how much strftime would output */
	size_t length = obstack_object_size(&ob) + 512;
	char *buffer = obstack_finish(&ob);
	char *out = obstack_alloc(&ob, length);
	strftime(out, length, buffer, date);
	length = put(out);
	obstack_free(&ob, buffer);
	return length;
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

static void format_calendar(int year, int month, int day, uint32_t links)
{
	struct tm date = {
		.tm_mday = 1,
		.tm_mon  = month,
		.tm_year = year,
	};
	if (mktime(&date) == -1)
		return;
	int last_day = month_length(&date);
	printft(snip_calendar_header, &date);
	while (date.tm_mday <= last_day) {
		if (date.tm_wday == 1 || date.tm_mday == 1)
			printft(snip_calendar_week, &date);
		if (date.tm_mday == 1) { /* pad */
			int dow = (date.tm_wday + 6) % 7;
			for (int i = 0; i < dow; i++)
				put(snip_calendar_empty_cell);
		}
		int today = date.tm_mday == day;
		int linked = links & (1 << (date.tm_mday - 1));
		printft(
			today  ? snip_calendar_current_day :
			linked ? snip_calendar_linked_day  :
			snip_calendar_regular_day,
			&date
		);
		if (date.tm_wday == 0)
			printft(snip_calendar_week_end, &date);
		/* increment */
		date.tm_mday++;
		date.tm_wday = (date.tm_wday + 1) % 7;
		date.tm_yday++;
	}
	if (date.tm_wday != 0) { /* pad */
		int dow = date.tm_wday - 1;
		for (int i = 0; i < 6 - dow; i++)
			put(snip_calendar_empty_cell);
		printft(snip_calendar_week_end, &date);
	}
	printft(snip_calendar_footer, &date);
}

static void html_calendars(struct context *ctx)
{
	put(snip_calendars_header);
	for (struct calendar *cal = ctx->calendars; cal; cal = cal->next) {
		int current_year = cal->year == ctx->date.tm_year;
		for (int i = 0; i < 12; i++) {
			int day = 0;
			if (current_year && i == ctx->date.tm_mon)
				day = ctx->date.tm_mday;
			if (cal->months[i] == 0)
				continue;
			format_calendar(cal->year, i, day, cal->months[i]);
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
	printft(snip_listing_header, &ctx->date);
	if (!entries) {
		put(snip_listing_empty);
		goto end;
	}
	for (; *entries; entries++) {
		printf(snip_listing_entry,
			ctx->base_uri,
			(*entries)->path,
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

void html_404()
{
	put(snip_404);
}

void html_main(struct context *ctx)
{
	obstack_init(&ob);
	printft(snip_header, &ctx->date, ctx->title, ctx->base_uri);
	html_calendars(ctx);
	html_listing(ctx);
	put(snip_footer);
	obstack_free(&ob, NULL);
}
