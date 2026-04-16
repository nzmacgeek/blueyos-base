/*
 * mkdir - make directories
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_parents = 0;
static mode_t opt_mode = 0755;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] DIRECTORY...\n", progname);
    fprintf(stderr, "Create the DIRECTORY(ies), if they do not already exist.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -p, --parents    Make parent directories as needed\n");
    fprintf(stderr, "  -m, --mode=MODE  Set file mode (as in chmod)\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int mkdir_parents(const char *path, mode_t mode) {
    char *copy = strdup(path);
    if (!copy) return -1;

    char *p = copy;
    if (*p == '/') p++;

    while (*p) {
        while (*p && *p != '/') p++;

        char save = *p;
        *p = '\0';

        if (*copy) {
            int rc = mkdir(copy, mode);
            if (rc < 0) {
                if (errno != EEXIST) {
                    fprintf(stderr, "mkdir: cannot create directory '%s': %s\n", copy, strerror(errno));
                    free(copy);
                    return -1;
                }

                struct stat st;
                if (stat(copy, &st) < 0) {
                    fprintf(stderr, "mkdir: cannot create directory '%s': %s\n", copy, strerror(errno));
                    free(copy);
                    return -1;
                }
                if (!S_ISDIR(st.st_mode)) {
                    fprintf(stderr, "mkdir: cannot create directory '%s': Not a directory\n", copy);
                    free(copy);
                    return -1;
                }
                if (verbose >= 1) {
                    printf("mkdir: directory '%s' already exists\n", copy);
                }
            } else {
                if (verbose >= 1) {
                    printf("mkdir: created directory '%s'\n", copy);
                }
            }
        }

        *p = save;
        if (*p) p++;
    }

    free(copy);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *dirs[256];
    int dir_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("mkdir version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--parents") == 0) {
            opt_parents = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strncmp(argv[i], "-m", 2) == 0) {
            const char *mode_str;
            if (argv[i][2] == '\0') {
                if (++i >= argc) {
                    fprintf(stderr, "mkdir: option requires an argument -- 'm'\n");
                    return 1;
                }
                mode_str = argv[i];
            } else {
                mode_str = argv[i] + 2;
            }
            opt_mode = (mode_t)strtol(mode_str, NULL, 8);
        } else if (strncmp(argv[i], "--mode=", 7) == 0) {
            opt_mode = (mode_t)strtol(argv[i] + 7, NULL, 8);
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "mkdir: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (dir_count < 256) dirs[dir_count++] = argv[i];
        }
    }

    if (dir_count == 0) {
        fprintf(stderr, "mkdir: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    int ret = 0;
    for (int i = 0; i < dir_count; i++) {
        if (opt_parents) {
            if (mkdir_parents(dirs[i], opt_mode) < 0) {
                ret = 1;
            }
        } else {
            if (mkdir(dirs[i], opt_mode) < 0) {
                fprintf(stderr, "mkdir: cannot create directory '%s': %s\n",
                        dirs[i], strerror(errno));
                ret = 1;
            } else if (verbose >= 1) {
                printf("mkdir: created directory '%s'\n", dirs[i]);
            }
        }
    }

    return ret;
}
