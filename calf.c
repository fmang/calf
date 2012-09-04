#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define __USE_XOPEN
#include <time.h>

struct cal_t {
    struct tm date;
    struct cal_t *next;
};

struct cal_t *first_day = 0;
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
    puts("<table><tr><th colspan=\"7\">");
    char buf[128];
    strftime(buf, 128, "%B %Y", &(cal->date));
    puts(buf);
    puts("</th></tr>");
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
                printf("<a href=\"%s/%s#listing\">%d</a>", root, buf, day);
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
    puts("</table>");
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

int main(int argc, char **argv){

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
                    "<h1>Oh noes, I can't find your file.</h1>"
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
                    puts("This is a directory");
                else if(st.st_size < 1024) // < 1k
                    puts("Not even a kilobyte, yay!");
                else if(st.st_size <= 9000) // 1k - 9k
                    puts("A few kilos. Watch over your disk space!");
                else if(st.st_size <= 20000) // 9k - 20k
                    puts("OVER 9000 BYTES");
                else if(st.st_size <= 700000) // 20k - 700k
                    printf("%lu kilos", st.st_size/1024);
                else if(st.st_size <= 2*1024*1024) // 700k - 2M
                    puts("About a meg or two");
                else if(st.st_size <= 10*1024*1024) // 2M - 10M
                    printf("%lu little megs", st.st_size/(1024*1024));
                else if(st.st_size <= 200000000) // 10M - 200M
                    printf("%lu megs. Getting big :o", st.st_size/(1024*1024));
                else if(st.st_size <= 900000000) // 200M - 900M
                    printf("%lu megs. Please don't hurt my bandwidth. I like my bandwidth :(", st.st_size/(1024*1024));
                else if(st.st_size <= (long unsigned int) 2*1024*1024*1024) // 900M - 2G
                    puts("A gig or two. You won't get away with this!");
                else
                    printf("%lu gigs. DON'T TOUCH IT!", st.st_size/(1024*1024*1024));
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
        puts("<span>And not a single fuck was given that day.</span>");
    else if(entry_count == 0)
        puts("<span>Lol, you thought something happened that day?</span>");
    free(entries);
    puts("</div>");

    puts(
                "</div>"
            "</body>"
        "</html>"
    );

    return EXIT_SUCCESS;

}
