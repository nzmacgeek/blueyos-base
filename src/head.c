/*
 * head - output the first part of files
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#define DEFAULT_LINES 10

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n N    Output the first N lines (default 10)\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static void head_file(FILE *fp, int nlines) {
    char buf[8192];
    int count = 0;
    while (count < nlines && fgets(buf, sizeof(buf), fp)) {
        fputs(buf, stdout);
        count++;
    }
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int nlines = DEFAULT_LINES;
    char *files[256];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("head version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "head: -n requires an argument\n");
                return 1;
            }
            nlines = atoi(argv[i]);
            if (nlines <= 0) {
                fprintf(stderr, "head: invalid number of lines '%s'\n", argv[i]);
                return 1;
            }
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "head: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        head_file(stdin, nlines);
    } else {
        for (int i = 0; i < file_count; i++) {
            FILE *fp = fopen(files[i], "r");
            if (!fp) {
                fprintf(stderr, "head: %s: %s\n", files[i], strerror(errno));
                continue;
            }
            if (file_count > 1) {
                printf("==> %s <==\n", files[i]);
            }
            head_file(fp, nlines);
            fclose(fp);
        }
    }

    return 0;
}
