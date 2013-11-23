#include "calf.h"
#include "htmlsnips.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Input/Output.
 * Formatting.
 */

static int put(const char *str)
{
	return fputs(str, stdout);
}

static int printft(const char *format, struct tm *date, ...)
{
	static char *buffer = NULL;
	if (!buffer)
		buffer = malloc(4096);
	va_list ap;
	va_start(ap, date);
	vsnprintf(buffer, 2048, format, ap);
	strftime(buffer + 2048, 2048, buffer, date);
	va_end(ap);
	return put(buffer + 2048);
}

void html_escape(const char *str)
{
	while (*str) {
		if (*str == '"') fputs("&quot;", stdout);
		else if (*str == '&') fputs("&amp;", stdout);
		else if (*str == '<') fputs("&lt;", stdout);
		else if (*str == '>') fputs("&gt;", stdout);
		else putchar(*str);
		str++;
	}
}

static void push(char **str, const char *c)
{
	size_t len = strlen(c);
	memcpy(*str, c, len);
	*str += len;
}

static char *entities(const char *str)
{
	static char *buffer = NULL;
	if (!buffer)
		buffer = malloc(1024);
	char *cursor = buffer;
	for (; *str; str++) {
		if (*str == '"')
			push(&cursor, "&quot;");
		else if (*str == '&')
			push(&cursor, "&amp;");
		else if (*str == '<')
			push(&cursor, "&lt;");
		else if (*str == '>')
			push(&cursor, "&gt;");
		else
			*(cursor++) = *str;
	}
	*cursor = '\0';
	return buffer;
}

/*******************************************************************************
 * Date-related functions.
 * Calendar generation.
 */

static int is_leap_year(struct tm *date)
{
	if (date->tm_year % 400 == 0) return 1;
	if (date->tm_year % 100 == 0) return 0;
	if (date->tm_year % 4 == 0) return 1;
	return 0;
}

static int days_for_month(struct tm *date)
{
	int month = date->tm_mon + 1;
	if (month == 2) return is_leap_year(date) ? 29 : 28;
	if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
	return 31;
}

static int next_mday(struct tm *date)
{
	if (date->tm_mday >= days_for_month(date))
		return -1;
	date->tm_mday++;
	date->tm_wday = (date->tm_wday + 1) % 7;
	date->tm_yday++;
	return 0;
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
	printft(snip_calendar_header, &date);
	do {
		if (date.tm_wday == 1 || date.tm_mday == 1)
			printft(snip_calendar_week, &date);
		if (date.tm_mday == 1) {
			int dow = (date.tm_wday + 6) % 7;
			for (int i = 0; i < dow; i++)
				put(snip_calendar_empty_cell);
		}
		if (date.tm_mday == day)
			printft(snip_calendar_current_day, &date);
		else if (links & (1 << (date.tm_mday - 1)))
			printft(snip_calendar_linked_day, &date);
		else
			printft(snip_calendar_regular_day, &date);
		if (date.tm_wday == 0)
			printft(snip_calendar_week_end, &date);
	} while (next_mday(&date) == 0);
	if (date.tm_wday != 0) {
		int dow = date.tm_wday - 1;
		for (int i = 0; i < 6 - dow; i++)
			put(snip_calendar_empty_cell);
		printft(snip_calendar_week_end, &date);
	}
	printft(snip_calendar_footer, &date);
}

void html_calendars(struct context *ctx)
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
	static char buffer[128];
	size_t size = st->st_size;
	if (S_ISDIR(st->st_mode))
		return "[DIR]";
	else if (size < 1024)
		snprintf(buffer, 128, "%lu B", size);
	else if (size < 1024*1024)
		snprintf(buffer, 128, "%lu KB", size/1024);
	else if (size < 1024*1024*1024)
		snprintf(buffer, 128, "%lu MB", size/(1024*1024));
	else
		snprintf(buffer, 128, "%lu GB", size/(1024*1024*1024));
	return buffer;
}

void html_listing(struct context *ctx, struct entry **entries)
{
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
 * Simple fragments.
 */

void html_404()
{
	put(snip_404);
}

void html_header(struct context *ctx)
{
	printft(snip_header, &ctx->date, ctx->title, ctx->base_uri);
}

void html_footer()
{
	put(snip_footer);
}
