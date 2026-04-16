/*
 * touch - change file timestamps or create empty files
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <utime.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_no_create = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] FILE...\n", progname);
    fprintf(stderr, "Update the access and modification times of each FILE to current time.\n");
    fprintf(stderr, "A FILE argument that does not exist is created empty.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -c, --no-create  Do not create any files\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int touch_file(const char *filename) {
    struct stat st;

    /* Check if file exists */
    if (stat(filename, &st) == 0) {
        /* File exists, update timestamp */
        if (utime(filename, NULL) < 0) {
            fprintf(stderr, "touch: cannot touch '%s': %s\n", filename, strerror(errno));
            return -1;
        }
        if (verbose >= 1) {
            printf("touch: updated '%s'\n", filename);
        }
        return 0;
    }

    /* File doesn't exist */
    if (opt_no_create) {
        return 0;
    }

    /* Create the file */
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        fprintf(stderr, "touch: cannot touch '%s': %s\n", filename, strerror(errno));
        return -1;
    }
    close(fd);

    if (verbose >= 1) {
        printf("touch: created '%s'\n", filename);
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
            printf("touch version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--no-create") == 0) {
            opt_no_create = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "touch: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "touch: missing file operand\n");
        print_usage(argv[0]);
        return 1;
    }

    int ret = 0;
    for (int i = 0; i < file_count; i++) {
        if (touch_file(files[i]) < 0) {
            ret = 1;
        }
    }

    return ret;
}
