/*
 * uptime - show how long the system has been running
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("uptime version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else {
            fprintf(stderr, "uptime: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Read uptime from /proc/uptime */
    FILE *fp = fopen("/proc/uptime", "r");
    if (!fp) {
        fprintf(stderr, "uptime: cannot open /proc/uptime: %s\n", strerror(errno));
        return 1;
    }
    double uptime_secs = 0.0;
    fscanf(fp, "%lf", &uptime_secs);
    fclose(fp);

    /* Read load averages from /proc/loadavg */
    fp = fopen("/proc/loadavg", "r");
    if (!fp) {
        fprintf(stderr, "uptime: cannot open /proc/loadavg: %s\n", strerror(errno));
        return 1;
    }
    double load1 = 0.0, load5 = 0.0, load15 = 0.0;
    fscanf(fp, "%lf %lf %lf", &load1, &load5, &load15);
    fclose(fp);

    /* Current time */
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    if (!tm_now) {
        fprintf(stderr, "uptime: localtime failed\n");
        return 1;
    }

    /* Format uptime duration */
    long total_secs = (long)uptime_secs;
    long days = total_secs / 86400;
    long hours = (total_secs % 86400) / 3600;
    long mins = (total_secs % 3600) / 60;

    printf(" %02d:%02d:%02d up ", tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
    if (days > 0) {
        printf("%ld day%s, ", days, days == 1 ? "" : "s");
    }
    printf("%ld:%02ld,  load average: %.2f, %.2f, %.2f\n",
           hours, mins, load1, load5, load15);

    return 0;
}
