/*
 * cat - concatenate files and print on standard output
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_number = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE]...\n", progname);
    fprintf(stderr, "Concatenate FILE(s) to standard output.\n\n");
    fprintf(stderr, "With no FILE, or when FILE is -, read standard input.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n, --number     Number all output lines\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int cat_file(const char *filename, int *line_num) {
    int fd;
    if (strcmp(filename, "-") == 0) {
        fd = STDIN_FILENO;
    } else {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "cat: %s: %s\n", filename, strerror(errno));
            return -1;
        }
    }

    char buf[8192];
    ssize_t n;
    int at_line_start = 1;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (opt_number && at_line_start) {
                printf("%6d  ", (*line_num)++);
                at_line_start = 0;
            }
            putchar(buf[i]);
            if (buf[i] == '\n') {
                at_line_start = 1;
            }
        }
    }

    if (n < 0) {
        fprintf(stderr, "cat: %s: read error: %s\n", filename, strerror(errno));
        if (fd != STDIN_FILENO) close(fd);
        return -1;
    }

    if (fd != STDIN_FILENO) {
        close(fd);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *files[256];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("cat version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--number") == 0) {
            opt_number = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "cat: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    /* If no files specified, read stdin */
    if (file_count == 0) {
        files[file_count++] = "-";
    }

    int ret = 0;
    int line_num = 1;
    for (int i = 0; i < file_count; i++) {
        if (cat_file(files[i], &line_num) < 0) {
            ret = 1;
        }
    }

    return ret;
}
