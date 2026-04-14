/*
 * print - print arguments to stdout
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [STRING...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n    Do not output trailing newline\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int no_newline = 0;
    int args_start = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("print version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
            args_start = i + 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            no_newline = 1;
            args_start = i + 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
            args_start = i + 1;
        } else {
            args_start = i;
            break;
        }
    }

    for (int i = args_start; i < argc; i++) {
        if (i > args_start) printf(" ");
        printf("%s", argv[i]);
    }

    if (!no_newline) printf("\n");

    return 0;
}
