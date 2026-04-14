/*
 * find - search for files in a directory hierarchy
 * Part of BlueyOS base utilities
 *
 * A simplified implementation of the UNIX find command
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fnmatch.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static char *name_pattern = NULL;
static int type_filter = 0; /* 0=all, 'f'=file, 'd'=directory */
static int maxdepth = -1;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [path...] [options]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -name PATTERN    Find files matching pattern\n");
    fprintf(stderr, "  -type TYPE       Filter by type (f=file, d=directory)\n");
    fprintf(stderr, "  -maxdepth N      Descend at most N levels\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int should_print(const char *name, struct stat *st) {
    /* Check type filter */
    if (type_filter == 'f' && !S_ISREG(st->st_mode)) {
        return 0;
    }
    if (type_filter == 'd' && !S_ISDIR(st->st_mode)) {
        return 0;
    }

    /* Check name pattern */
    if (name_pattern && fnmatch(name_pattern, name, 0) != 0) {
        return 0;
    }

    return 1;
}

static void find_recursive(const char *path, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char fullpath[4096];

    if (maxdepth >= 0 && depth > maxdepth) {
        return;
    }

    if (lstat(path, &st) < 0) {
        if (verbose >= 1) {
            fprintf(stderr, "find: cannot stat '%s': %s\n", path, strerror(errno));
        }
        return;
    }

    /* Print current path if it matches criteria */
    {
        const char *base = strrchr(path, '/');
        base = base ? base + 1 : path;
        if (should_print(base, &st)) {
            printf("%s\n", path);
        }
    }

    /* If not a directory, we're done */
    if (!S_ISDIR(st.st_mode)) {
        return;
    }

    /* Open and traverse directory */
    dir = opendir(path);
    if (!dir) {
        if (verbose >= 1) {
            fprintf(stderr, "find: cannot open directory '%s': %s\n", path, strerror(errno));
        }
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path */
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (lstat(fullpath, &st) < 0) {
            if (verbose >= 1) {
                fprintf(stderr, "find: cannot stat '%s': %s\n", fullpath, strerror(errno));
            }
            continue;
        }

        /* Recurse: for dirs, recurse into them; for files, call to print */
        find_recursive(fullpath, depth + 1);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    int i;
    int path_count = 0;
    char *paths[256];

    /* Check for VERBOSE environment variable */
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("find version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-name") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "find: -name requires an argument\n");
                return 1;
            }
            name_pattern = argv[i];
        } else if (strcmp(argv[i], "-type") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "find: -type requires an argument\n");
                return 1;
            }
            if (strlen(argv[i]) != 1 || (argv[i][0] != 'f' && argv[i][0] != 'd')) {
                fprintf(stderr, "find: -type argument must be 'f' or 'd'\n");
                return 1;
            }
            type_filter = argv[i][0];
        } else if (strcmp(argv[i], "-maxdepth") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "find: -maxdepth requires an argument\n");
                return 1;
            }
            maxdepth = atoi(argv[i]);
            if (maxdepth < 0) {
                fprintf(stderr, "find: -maxdepth must be non-negative\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "find: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            /* It's a path */
            if (path_count < 256) {
                paths[path_count++] = argv[i];
            }
        }
    }

    /* Default to current directory if no paths specified */
    if (path_count == 0) {
        paths[path_count++] = ".";
    }

    /* Process each path */
    for (i = 0; i < path_count; i++) {
        find_recursive(paths[i], 0);
    }

    return 0;
}
