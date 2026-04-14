/*
 * sort - sort lines of text files
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int numeric_sort = 0;
static int reverse_sort = 0;
static int unique_only = 0;

struct line_list {
    char **items;
    size_t count;
    size_t capacity;
};

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n              Compare according to numeric value\n");
    fprintf(stderr, "  -r              Reverse the result of comparisons\n");
    fprintf(stderr, "  -u              Output only the first of equal lines\n");
    fprintf(stderr, "  -o FILE         Write result to FILE instead of stdout\n");
    fprintf(stderr, "  -v, --verbose   Verbose output\n");
    fprintf(stderr, "  --version       Print version\n");
    fprintf(stderr, "  -h, --help      Show this help\n");
}

static int compare_lines(const void *a, const void *b) {
    const char *left = *(const char * const *)a;
    const char *right = *(const char * const *)b;
    int cmp;

    if (numeric_sort) {
        char *lend = NULL;
        char *rend = NULL;
        double lv = strtod(left, &lend);
        double rv = strtod(right, &rend);

        if (lend == left && rend == right) {
            cmp = strcmp(left, right);
        } else if (lv < rv) {
            cmp = -1;
        } else if (lv > rv) {
            cmp = 1;
        } else {
            cmp = strcmp(left, right);
        }
    } else {
        cmp = strcmp(left, right);
    }

    return reverse_sort ? -cmp : cmp;
}

static int append_line(struct line_list *lines, const char *line) {
    if (lines->count == lines->capacity) {
        size_t new_capacity = (lines->capacity == 0) ? 128 : lines->capacity * 2;
        char **new_items = realloc(lines->items, new_capacity * sizeof(char *));
        if (!new_items) {
            return -1;
        }
        lines->items = new_items;
        lines->capacity = new_capacity;
    }

    lines->items[lines->count] = strdup(line);
    if (!lines->items[lines->count]) {
        return -1;
    }

    lines->count++;
    return 0;
}

static int read_lines(FILE *fp, struct line_list *lines) {
    char line[8192];

    while (1) {
        errno = 0;
        if (fgets(line, sizeof(line), fp) != NULL) {
            if (append_line(lines, line) != 0) {
                return -1;
            }
            continue;
        }

        if (feof(fp)) {
            break;
        }

        if (ferror(fp) && errno == EINTR) {
            clearerr(fp);
            if (!isatty(fileno(fp))) {
                break;
            }
            continue;
        }

        return -1;
    }

    return 0;
}

static void free_lines(struct line_list *lines) {
    for (size_t i = 0; i < lines->count; i++) {
        free(lines->items[i]);
    }
    free(lines->items);
    lines->items = NULL;
    lines->count = 0;
    lines->capacity = 0;
}

static int write_sorted(FILE *out, struct line_list *lines) {
    if (lines->count == 0) {
        return 0;
    }

    qsort(lines->items, lines->count, sizeof(char *), compare_lines);

    const char *previous = NULL;
    for (size_t i = 0; i < lines->count; i++) {
        if (unique_only && previous && compare_lines(&previous, &lines->items[i]) == 0) {
            continue;
        }

        if (fputs(lines->items[i], out) == EOF) {
            return -1;
        }
        previous = lines->items[i];
    }

    return ferror(out) ? -1 : 0;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *files[256];
    int file_count = 0;
    const char *output_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("sort version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            numeric_sort = 1;
        } else if (strcmp(argv[i], "-r") == 0) {
            reverse_sort = 1;
        } else if (strcmp(argv[i], "-u") == 0) {
            unique_only = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "sort: -o requires an argument\n");
                return 1;
            }
            output_path = argv[i];
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "sort: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (file_count < 256) {
            files[file_count++] = argv[i];
        }
    }

    struct line_list lines = {0};
    int status = 0;

    if (file_count == 0) {
        if (read_lines(stdin, &lines) != 0) {
            fprintf(stderr, "sort: failed to read stdin: %s\n", strerror(errno));
            free_lines(&lines);
            return 1;
        }
    } else {
        for (int i = 0; i < file_count; i++) {
            FILE *fp = fopen(files[i], "r");
            if (!fp) {
                fprintf(stderr, "sort: %s: %s\n", files[i], strerror(errno));
                status = 1;
                continue;
            }

            if (verbose >= 2) {
                fprintf(stderr, "sort: reading '%s'\n", files[i]);
            }

            if (read_lines(fp, &lines) != 0) {
                fprintf(stderr, "sort: failed to read '%s': %s\n", files[i], strerror(errno));
                fclose(fp);
                free_lines(&lines);
                return 1;
            }

            fclose(fp);
        }
    }

    FILE *out = stdout;
    if (output_path) {
        out = fopen(output_path, "w");
        if (!out) {
            fprintf(stderr, "sort: %s: %s\n", output_path, strerror(errno));
            free_lines(&lines);
            return 1;
        }
    }

    if (write_sorted(out, &lines) != 0) {
        fprintf(stderr, "sort: write failed: %s\n", strerror(errno));
        status = 1;
    }

    if (out != stdout) {
        if (fclose(out) != 0) {
            fprintf(stderr, "sort: failed to close output file: %s\n", strerror(errno));
            status = 1;
        }
    }

    free_lines(&lines);
    return status;
}