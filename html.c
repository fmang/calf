#include "calf.h"
#include "snips.h"

static int is_leap_year(struct tm *date)
{
	if(date->tm_year % 400 == 0) return 1;
	if(date->tm_year % 100 == 0) return 0;
	if(date->tm_year % 4 == 0) return 1;
	return 0;
}

static int days_for_month(struct tm *date)
{
	int month = date->tm_mon + 1;
	if(month == 2) return is_leap_year(date) ? 29 : 28;
	if(month == 4 || month == 6 || month == 9 || month == 11) return 30;
	return 31;
}

void html_escape(const char *str)
{
	while(*str) {
		if(*str == '"') fputs("&quot;", stdout);
		else if(*str == '&') fputs("&amp;", stdout);
		else if(*str == '<') fputs("&lt;", stdout);
		else if(*str == '>') fputs("&gt;", stdout);
		else putchar(*str);
		str++;
	}
}

struct cal_t* html_calendar(struct cal_t *cal)
{
	int year = cal->date.tm_year, month = cal->date.tm_mon;
	int dow = (cal->date.tm_wday - cal->date.tm_mday + 7*5)%7;
	int day = 1, last_day = days_for_month(&(cal->date));
	int i = 0;
	fputs("<div class=\"calendar", stdout);
	if(year == current_date.tm_year && month == current_date.tm_mon)
		fputs(" current", stdout);
	puts("\"><h3>");
	char buf[128];
	strftime(buf, 128, "%B %Y", &(cal->date));
	puts(buf);
	puts("</h3><table>");
	if(dow != 0) {
		puts("<tr>");
		for(; i < dow; i++) puts("<td></td>");
	}
	for(; day <= last_day; day++, dow = (dow+1)%7) {
		if(dow == 0)
			puts("<tr>");
		puts(year == current_date.tm_year
		     && month == current_date.tm_mon
		     && day == current_date.tm_mday
		     ? "<td class=\"current\">" : "<td>");
		if(cal) {
			if(cal->date.tm_mday == day) {
				strftime(buf, 128, "%F", &(cal->date));
				printf("<a href=\"%s/%s\">%d</a>", base_uri, buf, day);
				cal = cal->next;
			} else printf("%d", day);
		} else {
			printf("%d", day);
		}
		puts("</td>");
		if(dow == 6)
			puts("</tr>");
	}
	if(dow != 0) {
		for(; dow < 7; dow++)
			puts("<td></td>");
		puts("</tr>");
	}
	puts("</table></div>");
	return cal;
}

void html_404()
{
	puts(snip_html_404);
}
