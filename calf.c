#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define __USE_XOPEN
#include <time.h>
#include <fcgi_stdio.h>

struct cal_t {
    struct tm date;
    struct cal_t *next;
};

struct tm current_date;
const char *root;

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
    int month = date->tm_mon + 1;
    if(month == 2) return is_leap_year(date) ? 29 : 28;
    if(month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

struct cal_t* print_html_calendar(struct cal_t *cal){
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
    if(dow != 0){
        puts("<tr>");
        for(; i < dow; i++) puts("<td></td>");
    }
    for(; day <= last_day; day++, dow = (dow+1)%7){
        if(dow == 0) puts("<tr>");
        puts(year == current_date.tm_year
             && month == current_date.tm_mon
             && day == current_date.tm_mday
            ? "<td class=\"current\">" : "<td>");
        if(cal){
            if(cal->date.tm_mday == day){
                strftime(buf, 128, "%F", &(cal->date));
                printf("<a href=\"%s/%s\">%d</a>", root, buf, day);
                cal = cal->next;
            }
            else printf("%d", day);
        }
        else printf("%d", day);
        puts("</td>");
        if(dow == 6) puts("</tr>");
    }
    if(dow != 0){
        for(; dow < 7; dow++) puts("<td></td>");
        puts("</tr>");
    }
    puts("</table></div>");
    return cal;
}

int set_current_date(){
    const char *uri = getenv("DOCUMENT_URI");
    if(strncmp(root, uri, strlen(root)) != 0)
        return 0;
    uri += strlen(root);
    char *pos;
    if((pos = strptime(uri, "/%F", &current_date))){
        if(*pos == '\0' || strcmp(pos, "/") == 0)
            return 1;
    }
    else if(*uri == '\0' || strcmp(uri, "/") == 0){
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

int process(){

    if(!getenv("CALF_ROOT")){
        fputs("No CALF_ROOT set.\n", stderr);
        return EXIT_FAILURE;
    }
    chdir(getenv("CALF_ROOT"));

    root = getenv("CALF_URI");
    if(!root) root = "";
    if(!set_current_date()){
        puts(
            "Content-Type: text/html\n"
            "Status: 404 Not Found\n"
            "\n"
            "<!doctype html>"
            "<html>"
                "<head>"
                    "<title>404 Not Found</title>"
                    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
                "</head>"
                "<body>"
                    "<h1>404 Not Found</h1>"
                "</body>"
            "</html>"
        );
        return EXIT_SUCCESS;
    }

    char *title = getenv("CALF_TITLE");
    if(!title) title = "Calf";
    char buf[128];
    strftime(buf, 128, "%B %-d, %Y", &current_date);
    printf(
        "Content-Type: text/html\n"
        "\n"
        "<!doctype html>"
        "<html>"
            "<head>"
                "<title>%s - %s</title>"
                "<link rel=\"stylesheet\" type=\"text/css\" href=\"%s/calf.css\" />"
                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
            "</head>"
            "<body>"
                "<div id=\"main\">",
        buf, title, root
    );

    struct dirent **entries;
    int entry_count = scandir(".", &entries, is_visible, alphasort);
    int i = 0;
    struct cal_t *first_day = 0, *current_cal = 0, *new_cal;
    struct tm date;
    char *pos;
    for(; i < entry_count; i++){
        if((pos = strptime(entries[i]->d_name, "%F", &date))){
            if(*pos == '\0'){
                new_cal = (struct cal_t*) malloc(sizeof(struct cal_t));
                memcpy(&(new_cal->date), &date, sizeof(struct tm));
                if(current_cal){
                    if(current_cal->date.tm_year == date.tm_year && current_cal->date.tm_mon == date.tm_mon){
                        new_cal->next = current_cal->next;
                        current_cal->next = new_cal;
                        current_cal = new_cal;
                    }
                    else{
                        new_cal->next = first_day;
                        first_day = current_cal = new_cal;
                    }
                }
                else{
                    new_cal->next = 0;
                    first_day = current_cal = new_cal;
                }
            }
        }
        free(entries[i]);
    }
    free(entries);
    entries = 0;

    puts("<div id=\"calendars\">");
    struct cal_t *current_day = first_day;
    while(current_day)
        current_day = print_html_calendar(current_day);
    puts("</div>");
    free_cal(first_day);

    puts("<div id=\"listing\">");
    printf("<h2>%s</h2>", buf);
    strftime(buf, 128, "%F", &current_date);
    entry_count = scandir(buf, &entries, is_visible, alphasort);
    if(entry_count > 0){
        char *path = 0;
        struct stat st;
        puts("<ul>");
        for(i = 0; i < entry_count; i++){
            puts("<li>");
            path = (char*) realloc(path, strlen(entries[i]->d_name) + 12);
            strncpy(path, buf, 10);
            path[10] = '/';
            strcpy(path+11, entries[i]->d_name);
            printf("<a href=\"%s/%s/", root, buf);
            print_escaped(entries[i]->d_name);
            printf("\">");
            if(stat(path, &st) == 0){
                puts("<span class=\"size\">");
                if(S_ISDIR(st.st_mode))
                    fputs("[DIR]", stdout);
                else if(st.st_size < 1024)
                    printf("%lu B", st.st_size);
                else if(st.st_size < 1024*1024)
                    printf("%lu KB", st.st_size/1024);
                else if(st.st_size < 1024*1024)
                    printf("%lu MB", st.st_size/(1024*1024));
                else
                    printf("%lu GB", st.st_size/(1024*1024*1024));
                puts("</span>");
            }
            print_escaped(entries[i]->d_name);
            puts("</a></li>");
            free(entries[i]);
        }
        puts("</ul>");
        if(path) free(path);
    }
    else if(entry_count < 0)
        puts("<span>Nothing.</span>");
    else if(entry_count <= 0)
        puts("<span>Well&hellip;</span>");
    free(entries);
    puts("</div><div style=\"clear:both;\"></div>");

    puts(
                "</div>"
            "</body>"
        "</html>"
    );

    return EXIT_SUCCESS;

}

int main(int argc, char **argv){
    while(!FCGI_Accept()){
        FCGI_SetExitStatus(process());
        FCGI_Finish();
    }
    return EXIT_SUCCESS;
}
