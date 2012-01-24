#include "common.h"
#include "rated.h"

/* convert a timeval to an ISO 8601 format string, return the length of the string */
size_t tv2iso8601(char *iso, struct timeval tv) {
    size_t len;
    time_t secs = tv.tv_sec;
    struct tm *tm = localtime(&secs);

    /* 1970-01-30T13:25:01.001 == 23, plus terminating NUL */
    len = strftime(iso, 24, ISO8601, tm);
    assert(len == 23);

    /* manually append milliseconds */
    len = sprintf(&iso[20], "%03ld", tv.tv_usec / 1000);
    assert(len == 3);

    return(strlen(iso));
}
