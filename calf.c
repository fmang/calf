#include <stdlib.h>
#include <stdio.h>
#include <time.h>

void day_of_week(int year, int month, int day){
    struct tm date;
    date.tm_hour = date.tm_min = date.tm_sec = 0;
    date.tm_year = year-1900;
    date.tm_mon = month-1;
    date.tm_mday = day;
    mktime(&date);
    char buf[256];
    strftime(buf, 256, "%F", &date);
    printf("Date: %s\n", buf);
    printf("Day of week: %d\n", date.tm_wday);
}

int main(int argc, char **argv){
    day_of_week(2012, 9, 2);
    return EXIT_SUCCESS;
}
