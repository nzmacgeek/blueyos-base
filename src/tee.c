/*
 * tee - read from stdin and write to stdout and files
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a    Append to files instead of overwriting\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int opt_append = 0;
    char *files[256];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("tee version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-a") == 0) {
            opt_append = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "tee: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    const char *mode = opt_append ? "a" : "w";
    FILE *fps[256];
    int open_count = 0;

    for (int i = 0; i < file_count; i++) {
        fps[i] = fopen(files[i], mode);
        if (!fps[i]) {
            fprintf(stderr, "tee: %s: %s\n", files[i], strerror(errno));
        } else {
            open_count++;
        }
    }

    char buf[8192];
    size_t n;
    int ret = 0;

    while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        /* Write to stdout */
        if (fwrite(buf, 1, n, stdout) != n) {
            fprintf(stderr, "tee: write error: %s\n", strerror(errno));
            ret = 1;
        }
        /* Write to each file */
        for (int i = 0; i < file_count; i++) {
            if (fps[i] && fwrite(buf, 1, n, fps[i]) != n) {
                fprintf(stderr, "tee: %s: write error: %s\n", files[i], strerror(errno));
                ret = 1;
            }
        }
    }

    for (int i = 0; i < file_count; i++) {
        if (fps[i]) fclose(fps[i]);
    }

    return ret;
}
