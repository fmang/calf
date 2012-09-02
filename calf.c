#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#define __USE_XOPEN
#include <time.h>

struct cal {
    int year, month, day;
    struct cal *next;
};

struct cal *first_day = 0;

void free_cal(struct cal *cur){
    struct cal *next;
    while(cur){
        next = cur->next;
        free(cur);
        cur = next;
    }
}

int is_leap_year(int year){
    if(year % 400 == 0) return 1;
    if(year % 100 == 0) return 0;
    if(year % 4 == 0) return 1;
    return 0;
}

int days_for_month(int year, int month){
    if(month == 2) return is_leap_year(year) ? 29 : 28;
    if(month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

struct tm mkdate(int year, int month, int day){
    struct tm date;
    date.tm_hour = date.tm_min = date.tm_sec = 0;
    date.tm_year = year-1900;
    date.tm_mon = month-1;
    date.tm_mday = day;
    mktime(&date);
    return date;
}

void print_html_calendar(int year, int month){
    struct tm date = mkdate(year, month, 1);
    int dow = date.tm_wday;
    int day = 1, last_day = days_for_month(year, month);
    int i = 0;
    puts("<table><tr><th colspan=\"7\">");
    char pretty_month[128];
    strftime(pretty_month, 128, "%B %Y", &date);
    puts(pretty_month);
    puts("</th>");
    if(dow != 0){
        puts("<tr>");
        for(; i < dow; i++) puts("<td></td>");
    }
    for(; day <= last_day; day++, dow = (dow+1)%7){
        if(dow == 0) puts("<tr>");
        printf("<td>%d</td>", day);
        if(dow == 6) puts("</tr>");
    }
    if(dow < 6){
        for(; dow < 7; dow++) puts("<td></td>");
        puts("</tr>");
    }
    puts("</table>");
}

int main(int argc, char **argv){
    puts(
        "Content-Type: text/html\n"
        "\n"
        "<!doctype html>"
        "<html>"
            "<head>"
                "<title>Calf</title>"
            "</head>"
            "<body>"
    );
    print_html_calendar(2012, 9);
    struct dirent **entries;
    int entry_count = scandir(".", &entries, 0, alphasort);
    int i = 0;
    struct cal **current_day = &first_day;
    struct tm date;
    char *pos;
    for(; i < entry_count; i++){
        if((pos = strptime(entries[i]->d_name, "%F", &date))){
            if(*pos == '\0'){
                *current_day = (struct cal*) malloc(sizeof(struct cal));
                (*current_day)->year = date.tm_year;
                (*current_day)->month = date.tm_mon;
                (*current_day)->day = date.tm_mday;
                (*current_day)->next = 0;
                current_day = &((*current_day)->next);
            }
        }
        free(entries[i]);
    }
    free(entries);
    puts("</ul>");
    puts(
            "</body>"
        "<html>"
    );
    free_cal(first_day);
    return EXIT_SUCCESS;
}
