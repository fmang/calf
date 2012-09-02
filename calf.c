#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#define __USE_XOPEN
#include <time.h>

struct cal_t {
    struct tm date;
    struct cal_t *next;
};

struct cal_t *first_day = 0;

struct tm current_date;

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
    int year = cal->date.tm_year, month = cal->date.tm_mon;
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
            puts(year == current_date.tm_year
                 && month == current_date.tm_mon
                 && day == current_date.tm_mday
                ? "<td class=\"current\">" : "<td>");
        }
        else puts("<td>");
        if(cal){
            if(cal->date.tm_mday == day){
                strftime(buf, 128, "%F", &(cal->date));
                printf("<a href=\"/%s\">%d</a>", buf, day);
                cal = cal->next;
            }
            else printf("%d", day);
        }
        else printf("%d", day);
        puts("</td>");
        if(dow == 6) puts("</tr>");
    }
    if(dow < 6){
        for(; dow < 7; dow++) puts("<td></td>");
        puts("</tr>");
    }
    puts("</table>");
    return cal;
}

int set_current_date(){
    char *pos;
    if((pos = strptime(getenv("DOCUMENT_URI"), "/%F", &current_date))){
        if(*pos == '\0' || strcmp(pos, "/") == 0) return 1;
    }
    if(strcmp(getenv("DOCUMENT_URI"), "/") == 0){
        time_t now;
        time(&now);
        memcpy(&current_date, gmtime(&now), sizeof(struct tm));
        return 1;
    }
    return 0;
}

int is_visible(const struct dirent *entry){
    return entry->d_name[0] != '.';
}

void print_escaped(const char *str){
    while(*str){
        if(*str == '"') fputs("&quot;", stdout);
        else if(*str == '&') fputs("&amp;", stdout);
        else if(*str == '<') fputs("&lt;", stdout);
        else if(*str == '>') fputs("&gt;", stdout);
        else putchar(*str);
        str++;
    }
}

int main(int argc, char **argv){

    if(!getenv("CALF_ROOT")){
        fputs("No CALF_ROOT set.\n", stderr);
        return EXIT_FAILURE;
    }
    chdir(getenv("CALF_ROOT"));

    if(!set_current_date()){
        puts(
            "Content-Type: text/html\n"
            "Status: 404 Not Found\n"
            "\n"
            "<!doctype html>"
            "<html>"
                "<head>"
                    "<title>404 Not Found</title>"
                "</head>"
                "<body>"
                    "<h1>Oh noes, I can't find your file.</h1>"
                "</body>"
            "</html>"
        );
        return EXIT_SUCCESS;
    }

    puts(
        "Content-Type: text/html\n"
        "\n"
        "<!doctype html>"
        "<html>"
            "<head>"
                "<title>Calf</title>"
                "<link rel=\"stylesheet\" type=\"text/css\" href=\"/calf.css\" />"
            "</head>"
            "<body>"
    );

    struct dirent **entries;
    int entry_count = scandir(".", &entries, is_visible, alphasort);
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
    free_cal(first_day);

    char buf[128];
    strftime(buf, 128, "%F", &current_date);
    entry_count = scandir(buf, &entries, is_visible, alphasort);
    if(entry_count > 0){
        puts("<ul>");
        for(i = 0; i < entry_count; i++){
            printf("<li><a href=\"/%s/", buf);
            print_escaped(entries[i]->d_name);
            printf("\">");
            print_escaped(entries[i]->d_name);
            puts("</a></li>");
            free(entries[i]);
        }
        puts("</ul>");
    }
    else if(entry_count < 0)
        puts("<div>And not a single fuck was given that day.</div>");
    else if(entry_count == 0)
        puts("<div>Lol, you though something happened that day?</div>");
    free(entries);

    puts(
            "</body>"
        "<html>"
    );

    return EXIT_SUCCESS;

}
