/*
 * diff - compare files line by line
 * Part of BlueyOS base utilities
 * Basic implementation of unified diff format
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
static int opt_unified = 3;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] FILE1 FILE2\n", progname);
    fprintf(stderr, "Compare FILES line by line.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -u, --unified[=NUM]  Output NUM (default 3) lines of unified context\n");
    fprintf(stderr, "  -q, --brief          Report only when files differ\n");
    fprintf(stderr, "  -v, --verbose        Verbose output\n");
    fprintf(stderr, "  --version            Print version\n");
    fprintf(stderr, "  -h, --help           Show this help\n");
}

typedef struct {
    char **lines;
    int count;
} file_lines_t;

static file_lines_t read_file_lines(const char *filename) {
    file_lines_t result = {NULL, 0};
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "diff: %s: %s\n", filename, strerror(errno));
        return result;
    }

    int capacity = 1024;
    result.lines = malloc(capacity * sizeof(char *));
    if (!result.lines) {
        fclose(fp);
        return result;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fp)) != -1) {
        if (result.count >= capacity) {
            capacity *= 2;
            char **tmp = realloc(result.lines, capacity * sizeof(char *));
            if (!tmp) break;
            result.lines = tmp;
        }
        result.lines[result.count++] = strdup(line);
    }

    free(line);
    fclose(fp);
    return result;
}

static void free_lines(file_lines_t *fl) {
    for (int i = 0; i < fl->count; i++) {
        free(fl->lines[i]);
    }
    free(fl->lines);
}

static int files_identical(file_lines_t *f1, file_lines_t *f2) {
    if (f1->count != f2->count) return 0;
    for (int i = 0; i < f1->count; i++) {
        if (strcmp(f1->lines[i], f2->lines[i]) != 0) {
            return 0;
        }
    }
    return 1;
}

static void print_unified_diff(const char *file1, const char *file2,
                               file_lines_t *f1, file_lines_t *f2) {
    /* Simple line-by-line diff */
    printf("--- %s\n", file1);
    printf("+++ %s\n", file2);

    int i1 = 0, i2 = 0;
    while (i1 < f1->count || i2 < f2->count) {
        /* Find next difference */
        int start1 = i1, start2 = i2;
        while (i1 < f1->count && i2 < f2->count &&
               strcmp(f1->lines[i1], f2->lines[i2]) == 0) {
            i1++;
            i2++;
        }

        if (i1 >= f1->count && i2 >= f2->count) break;

        /* Found difference, find end */
        int diff_start1 = i1, diff_start2 = i2;
        while (i1 < f1->count || i2 < f2->count) {
            if (i1 < f1->count && i2 < f2->count &&
                strcmp(f1->lines[i1], f2->lines[i2]) == 0) {
                break;
            }
            if (i1 < f1->count) i1++;
            if (i2 < f2->count) i2++;
        }

        /* Print hunk header */
        int ctx_start1 = (diff_start1 > opt_unified) ? diff_start1 - opt_unified : 0;
        int ctx_end1 = (i1 + opt_unified < f1->count) ? i1 + opt_unified : f1->count;
        int ctx_start2 = (diff_start2 > opt_unified) ? diff_start2 - opt_unified : 0;
        int ctx_end2 = (i2 + opt_unified < f2->count) ? i2 + opt_unified : f2->count;

        printf("@@ -%d,%d +%d,%d @@\n",
               ctx_start1 + 1, ctx_end1 - ctx_start1,
               ctx_start2 + 1, ctx_end2 - ctx_start2);

        /* Print context and changes */
        for (int j = ctx_start1; j < diff_start1; j++) {
            printf(" %s", f1->lines[j]);
        }
        for (int j = diff_start1; j < i1; j++) {
            if (j < f1->count) printf("-%s", f1->lines[j]);
        }
        for (int j = diff_start2; j < i2; j++) {
            if (j < f2->count) printf("+%s", f2->lines[j]);
        }
        for (int j = i1; j < ctx_end1 && j < f1->count; j++) {
            printf(" %s", f1->lines[j]);
        }
    }
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int opt_brief = 0;
    char *file1 = NULL, *file2 = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("diff version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--unified") == 0) {
            opt_unified = 3;
            /* Check if next argument is a non-negative integer */
            if (i + 1 < argc) {
                char *end;
                long n = strtol(argv[i + 1], &end, 10);
                if (*end == '\0' && n >= 0) {
                    opt_unified = (int)n;
                    i++;
                }
            }
        } else if (strncmp(argv[i], "-u", 2) == 0 && argv[i][2] >= '0' && argv[i][2] <= '9') {
            char *end;
            long n = strtol(argv[i] + 2, &end, 10);
            if (*end != '\0' || n < 0) {
                fprintf(stderr, "diff: invalid argument for -u: '%s'\n", argv[i] + 2);
                return 1;
            }
            opt_unified = (int)n;
        } else if (strncmp(argv[i], "--unified=", 10) == 0) {
            char *end;
            long n = strtol(argv[i] + 10, &end, 10);
            if (*end != '\0' || n < 0) {
                fprintf(stderr, "diff: invalid argument for --unified: '%s'\n", argv[i] + 10);
                return 1;
            }
            opt_unified = (int)n;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--brief") == 0) {
            opt_brief = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "diff: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (!file1) file1 = argv[i];
            else if (!file2) file2 = argv[i];
            else {
                fprintf(stderr, "diff: extra operand '%s'\n", argv[i]);
                return 1;
            }
        }
    }

    if (!file1 || !file2) {
        fprintf(stderr, "diff: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    file_lines_t f1 = read_file_lines(file1);
    file_lines_t f2 = read_file_lines(file2);

    if (!f1.lines || !f2.lines) {
        if (f1.lines) free_lines(&f1);
        if (f2.lines) free_lines(&f2);
        return 2;
    }

    if (files_identical(&f1, &f2)) {
        free_lines(&f1);
        free_lines(&f2);
        return 0;
    }

    if (opt_brief) {
        printf("Files %s and %s differ\n", file1, file2);
    } else {
        print_unified_diff(file1, file2, &f1, &f2);
    }

    free_lines(&f1);
    free_lines(&f2);
    return 1;
}
