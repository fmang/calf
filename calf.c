#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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

int day_of_week(int year, int month, int day){
    struct tm date;
    date.tm_hour = date.tm_min = date.tm_sec = 0;
    date.tm_year = year-1900;
    date.tm_mon = month-1;
    date.tm_mday = day;
    mktime(&date);
    return date.tm_wday;
}

int main(int argc, char **argv){
    printf("Day of week: %d\n", day_of_week(2012, 9, 2));
    return EXIT_SUCCESS;
}
