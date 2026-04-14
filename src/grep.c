/*
 * grep - print lines matching a pattern
 * Part of BlueyOS base utilities
 *
 * A simplified implementation of the UNIX grep command
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif
#define MAX_LINE 8192

static int verbose = 0;
static int show_line_numbers = 0;
static int show_filename = 1;
static int count_only = 0;
static int invert_match = 0;
static int ignore_case = 0;
static int use_extended = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] PATTERN [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n              Show line numbers\n");
    fprintf(stderr, "  -c              Count matching lines only\n");
    fprintf(stderr, "  -v              Invert match (select non-matching lines)\n");
    fprintf(stderr, "  -i              Ignore case\n");
    fprintf(stderr, "  -E              Use extended regular expressions\n");
    fprintf(stderr, "  -h              Suppress filename prefix\n");
    fprintf(stderr, "  --verbose       Verbose output\n");
    fprintf(stderr, "  --version       Print version\n");
    fprintf(stderr, "  --help          Show this help\n");
}

static int grep_file(FILE *fp, const char *filename, regex_t *regex) {
    char line[MAX_LINE];
    int line_num = 0;
    int match_count = 0;
    int ret;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line_num++;

        /* Remove trailing newline for cleaner output */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        /* Test match */
        ret = regexec(regex, line, 0, NULL, 0);
        int matches = (ret == 0);

        /* Apply invert flag */
        if (invert_match) {
            matches = !matches;
        }

        if (matches) {
            match_count++;

            if (!count_only) {
                if (filename && show_filename) {
                    printf("%s:", filename);
                }
                if (show_line_numbers) {
                    printf("%d:", line_num);
                }
                printf("%s\n", line);
            }
        }
    }

    if (count_only) {
        if (filename && show_filename) {
            printf("%s:", filename);
        }
        printf("%d\n", match_count);
    }

    return match_count;
}

int main(int argc, char *argv[]) {
    int i;
    char *pattern = NULL;
    int file_count = 0;
    char *files[256];
    regex_t regex;
    int regex_flags = REG_NOSUB;
    int total_matches = 0;

    /* Check for VERBOSE environment variable */
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("grep version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            show_line_numbers = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            count_only = 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            invert_match = 1;
        } else if (strcmp(argv[i], "-i") == 0) {
            ignore_case = 1;
        } else if (strcmp(argv[i], "-E") == 0) {
            use_extended = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            show_filename = 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "grep: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        } else if (!pattern) {
            pattern = argv[i];
        } else {
            /* It's a file */
            if (file_count < 256) {
                files[file_count++] = argv[i];
            }
        }
    }

    if (!pattern) {
        fprintf(stderr, "grep: no pattern specified\n");
        print_usage(argv[0]);
        return 2;
    }

    /* Set up regex flags */
    if (ignore_case) {
        regex_flags |= REG_ICASE;
    }
    if (use_extended) {
        regex_flags |= REG_EXTENDED;
    }

    /* Compile regex */
    int ret = regcomp(&regex, pattern, regex_flags);
    if (ret != 0) {
        char errbuf[256];
        regerror(ret, &regex, errbuf, sizeof(errbuf));
        fprintf(stderr, "grep: invalid pattern: %s\n", errbuf);
        return 2;
    }

    /* If multiple files, show filenames */
    if (file_count > 1) {
        show_filename = 1;
    }

    /* Process files or stdin */
    if (file_count == 0) {
        /* Read from stdin */
        show_filename = 0;
        total_matches = grep_file(stdin, NULL, &regex);
    } else {
        /* Process each file */
        for (i = 0; i < file_count; i++) {
            FILE *fp = fopen(files[i], "r");
            if (!fp) {
                fprintf(stderr, "grep: %s: %s\n", files[i], strerror(errno));
                continue;
            }

            if (verbose >= 2) {
                fprintf(stderr, "grep: processing file '%s'\n", files[i]);
            }

            total_matches += grep_file(fp, files[i], &regex);
            fclose(fp);
        }
    }

    regfree(&regex);

    /* Exit status: 0 if match found, 1 if no match, 2 if error */
    return (total_matches > 0) ? 0 : 1;
}
