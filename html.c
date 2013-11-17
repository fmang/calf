#include "calf.h"
#include "snips.h"

int put(const char *str)
{
	return fputs(str, stdout);
}

int tput(const char *format, struct tm *date)
{
	static char buffer[512];
	strftime(buffer, 512, format, date);
	return put(buffer);
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

void html_cal(int year, int month, int day)
{
	struct tm date = {
		.tm_mday = 1,
		.tm_mon  = month - 1,
		.tm_year = year - 1900,
	};
	if (mktime(&date) == -1)
		return;
	tput(snip_calendar_header, &date);
	do {
		if (date.tm_wday == 1 || date.tm_mday == 1)
			tput(snip_calendar_week, &date);
		if (date.tm_mday == 1) {
			int dow = (date.tm_wday + 6) % 7;
			for (int i = 0; i < dow; i++)
				put(snip_calendar_empty_cell);
		}
		if (date.tm_mday == day)
			tput(snip_calendar_current_day, &date);
		else
			tput(snip_calendar_regular_day, &date);
		if (date.tm_wday == 0)
			tput(snip_calendar_week_end, &date);
	} while (next_mday(&date) == 0);
	if (date.tm_wday != 0) {
		int dow = date.tm_wday - 1;
		for (int i = 0; i < 6 - dow; i++)
			put(snip_calendar_empty_cell);
		tput(snip_calendar_week_end, &date);
	}
	tput(snip_calendar_footer, &date);
}

struct cal_t* html_calendar(struct cal_t *cal)
{
	int year = cal->date.tm_year, month = cal->date.tm_mon;
	int dow = (cal->date.tm_wday - cal->date.tm_mday + 7*5)%7;
	int day = 1, last_day = days_for_month(&(cal->date));
	int i = 0;
	fputs("<div class=\"calendar", stdout);
	if (year == current_date.tm_year && month == current_date.tm_mon)
		fputs(" current", stdout);
	puts("\"><h3>");
	char buf[128];
	strftime(buf, 128, "%B %Y", &(cal->date));
	puts(buf);
	puts("</h3><table>");
	if (dow != 0) {
		puts("<tr>");
		for (; i < dow; i++)
			puts("<td></td>");
	}
	for (; day <= last_day; day++, dow = (dow+1)%7) {
		if (dow == 0)
			puts("<tr>");
		puts(year == current_date.tm_year
		     && month == current_date.tm_mon
		     && day == current_date.tm_mday
		     ? "<td class=\"current\">" : "<td>");
		if (cal) {
			if (cal->date.tm_mday == day) {
				strftime(buf, 128, "%F", &(cal->date));
				printf("<a href=\"%s/%s\">%d</a>", base_uri, buf, day);
				cal = cal->next;
			} else {
				printf("%d", day);
			}
		} else {
			printf("%d", day);
		}
		puts("</td>");
		if (dow == 6)
			puts("</tr>");
	}
	if (dow != 0) {
		for (; dow < 7; dow++)
			puts("<td></td>");
		puts("</tr>");
	}
	puts("</table></div>");
	return cal;
}

void html_404()
{
	put(snip_404);
}

void html_header(const char *title, const char *base_uri, const char *date)
{
	printf(snip_header, date, title, base_uri);
}

void html_footer()
{
	put(snip_footer);
}
