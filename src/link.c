/*
 * link - create a hard link
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s TARGET LINK_NAME\n", progname);
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
            printf("link version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    char *target = NULL;
    char *link_name = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "link: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (!target) {
            target = argv[i];
        } else if (!link_name) {
            link_name = argv[i];
        } else {
            fprintf(stderr, "link: too many arguments\n");
            return 1;
        }
    }

    if (!target || !link_name) {
        fprintf(stderr, "link: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    if (link(target, link_name) < 0) {
        fprintf(stderr, "link: cannot create link '%s' -> '%s': %s\n",
                link_name, target, strerror(errno));
        return 1;
    }

    if (verbose >= 1) {
        printf("'%s' -> '%s'\n", link_name, target);
    }

    return 0;
}
