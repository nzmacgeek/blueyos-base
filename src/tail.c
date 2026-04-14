/*
 * tail - output the last part of files
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

#define DEFAULT_LINES 10

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n N    Output the last N lines (default 10)\n");
    fprintf(stderr, "  -f      Follow: keep reading as file grows\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static void tail_file(FILE *fp, int nlines) {
    /* Store last N lines in a ring buffer */
    char **ring = malloc((size_t)nlines * sizeof(char *));
    if (!ring) {
        fprintf(stderr, "tail: out of memory\n");
        return;
    }
    for (int i = 0; i < nlines; i++) ring[i] = NULL;

    int head = 0, count = 0;
    char buf[8192];

    while (fgets(buf, sizeof(buf), fp)) {
        free(ring[head]);
        ring[head] = strdup(buf);
        if (!ring[head]) {
            fprintf(stderr, "tail: out of memory\n");
            break;
        }
        head = (head + 1) % nlines;
        if (count < nlines) count++;
    }

    /* Print ring buffer in order */
    int start = (count < nlines) ? 0 : head;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % nlines;
        if (ring[idx]) fputs(ring[idx], stdout);
    }

    for (int i = 0; i < nlines; i++) free(ring[i]);
    free(ring);
}

static void follow_file(FILE *fp) {
    char buf[8192];
    while (1) {
        while (fgets(buf, sizeof(buf), fp)) {
            fputs(buf, stdout);
            fflush(stdout);
        }
        clearerr(fp);
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int nlines = DEFAULT_LINES;
    int follow = 0;
    char *files[256];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("tail version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-f") == 0) {
            follow = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "tail: -n requires an argument\n");
                return 1;
            }
            nlines = atoi(argv[i]);
            if (nlines <= 0) {
                fprintf(stderr, "tail: invalid number of lines '%s'\n", argv[i]);
                return 1;
            }
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "tail: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        tail_file(stdin, nlines);
        if (follow) follow_file(stdin);
    } else {
        for (int i = 0; i < file_count; i++) {
            FILE *fp = fopen(files[i], "r");
            if (!fp) {
                fprintf(stderr, "tail: %s: %s\n", files[i], strerror(errno));
                continue;
            }
            if (file_count > 1) {
                printf("==> %s <==\n", files[i]);
            }
            tail_file(fp, nlines);
            if (follow && i == file_count - 1) {
                follow_file(fp);
            }
            fclose(fp);
        }
    }

    return 0;
}
