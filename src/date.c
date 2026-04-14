/*
 * date - print or set the system date and time
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [+FORMAT | MMDDhhmm[[CC]YY]]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -u    Use UTC/GMT\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

/*
 * Parse MMDDhhmm[[CC]YY] format and set system time.
 * Requires root.
 */
static int set_date(const char *s) {
    size_t len = strlen(s);
    if (len != 8 && len != 10 && len != 12) {
        fprintf(stderr, "date: invalid date format '%s'\n", s);
        return 1;
    }

    char mm[3] = {s[0], s[1], 0};
    char dd[3] = {s[2], s[3], 0};
    char hh[3] = {s[4], s[5], 0};
    char mi[3] = {s[6], s[7], 0};

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (!t) return 1;

    t->tm_mon  = atoi(mm) - 1;
    t->tm_mday = atoi(dd);
    t->tm_hour = atoi(hh);
    t->tm_min  = atoi(mi);
    t->tm_sec  = 0;

    if (len == 10) {
        char yy[3] = {s[8], s[9], 0};
        t->tm_year = 2000 + atoi(yy) - 1900;
    } else if (len == 12) {
        char cc[3] = {s[8], s[9], 0};
        char yy[3] = {s[10], s[11], 0};
        t->tm_year = atoi(cc) * 100 + atoi(yy) - 1900;
    }

    t->tm_isdst = -1;
    time_t new_time = mktime(t);
    if (new_time == (time_t)-1) {
        fprintf(stderr, "date: invalid time specification\n");
        return 1;
    }

    struct timeval tv;
    tv.tv_sec = new_time;
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) < 0) {
        fprintf(stderr, "date: settimeofday: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("date version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    int opt_utc = 0;
    char *format = NULL;
    char *set_str = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0) {
            opt_utc = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '+') {
            format = argv[i] + 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "date: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            set_str = argv[i];
        }
    }

    if (set_str) {
        /* Setting date requires root */
        if (getuid() != 0 && geteuid() != 0) {
            fprintf(stderr, "date: permission denied\n");
            return 1;
        }
        return set_date(set_str);
    }

    /* Print date */
    time_t now = time(NULL);
    struct tm *t = opt_utc ? gmtime(&now) : localtime(&now);
    if (!t) {
        fprintf(stderr, "date: cannot get time\n");
        return 1;
    }

    char buf[256];
    const char *fmt = format ? format : "%a %b %e %H:%M:%S %Z %Y";
    strftime(buf, sizeof(buf), fmt, t);
    printf("%s\n", buf);

    return 0;
}
