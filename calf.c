#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#define __USE_XOPEN
#include <time.h>

struct cal_t {
    struct tm date;
    struct cal_t *next;
};

struct cal_t *first_day = 0;

void free_cal(struct cal_t *cur){
    struct cal_t *next;
    while(cur){
        next = cur->next;
        free(cur);
        cur = next;
    }
}

int is_leap_year(struct tm *date){
    if(date->tm_year % 400 == 0) return 1;
    if(date->tm_year % 100 == 0) return 0;
    if(date->tm_year % 4 == 0) return 1;
    return 0;
}

int days_for_month(struct tm *date){
    int month = date->tm_mon;
    if(month == 2) return is_leap_year(date) ? 29 : 28;
    if(month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

struct cal_t* print_html_calendar(struct cal_t *cal){
    int dow = (cal->date.tm_wday - cal->date.tm_mday + 7)%7;
    int day = 1, last_day = days_for_month(&(cal->date));
    int i = 0;
    puts("<table><tr><th colspan=\"7\">");
    char buf[128];
    strftime(buf, 128, "%B %Y", &(cal->date));
    puts(buf);
    puts("</th>");
    if(dow != 0){
        puts("<tr>");
        for(; i < dow; i++) puts("<td></td>");
    }
    for(; day <= last_day; day++, dow = (dow+1)%7){
        if(dow == 0) puts("<tr>");
        if(cal){
            if(cal->date.tm_mday == day){
                strftime(buf, 128, "%F", &(cal->date));
                printf("<td><a href=\"%s\">%d</a></td>", buf, day);
                cal = cal->next;
            }
            else
                printf("<td>%d</td>", day);
        }
        else
            printf("<td>%d</td>", day);
        if(dow == 6) puts("</tr>");
    }
    if(dow < 6){
        for(; dow < 7; dow++) puts("<td></td>");
        puts("</tr>");
    }
    puts("</table>");
    return cal;
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
    struct dirent **entries;
    int entry_count = scandir(".", &entries, 0, alphasort);
    int i = 0;
    struct cal_t **current_cal = &first_day;
    struct tm date;
    char *pos;
    for(; i < entry_count; i++){
        if((pos = strptime(entries[i]->d_name, "%F", &date))){
            if(*pos == '\0'){
                *current_cal = (struct cal_t*) malloc(sizeof(struct cal_t));
                memcpy(&((*current_cal)->date), &date, sizeof(struct tm));
                (*current_cal)->next = 0;
                current_cal = &((*current_cal)->next);
            }
        }
        free(entries[i]);
    }
    free(entries);
    struct cal_t *current_day = first_day;
    while(current_day)
        current_day = print_html_calendar(current_day);
    puts("</ul>");
    puts(
            "</body>"
        "<html>"
    );
    free_cal(first_day);
    return EXIT_SUCCESS;
}
