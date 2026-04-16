/*
 * rm - remove files and directories
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_recursive = 0;
static int opt_force = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] FILE...\n", progname);
    fprintf(stderr, "Remove (unlink) the FILE(s).\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -r, -R, --recursive  Remove directories and their contents\n");
    fprintf(stderr, "  -f, --force          Ignore nonexistent files, never prompt\n");
    fprintf(stderr, "  -v, --verbose        Verbose output\n");
    fprintf(stderr, "  --version            Print version\n");
    fprintf(stderr, "  -h, --help           Show this help\n");
}

static int remove_recursive(const char *path);

static int remove_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        if (!opt_force) {
            fprintf(stderr, "rm: cannot open '%s': %s\n", path, strerror(errno));
        }
        return -1;
    }

    int ret = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);

        if (remove_recursive(full_path) < 0) {
            ret = -1;
        }
    }

    closedir(dir);

    if (rmdir(path) < 0) {
        if (!opt_force) {
            fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
        }
        return -1;
    }

    if (verbose >= 1) {
        printf("removed directory '%s'\n", path);
    }

    return ret;
}

static int remove_recursive(const char *path) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        int err = errno;

        if (opt_force && err == ENOENT) {
            return 0;
        }

        if (!opt_force) {
            fprintf(stderr, "rm: cannot stat '%s': %s\n", path, strerror(err));
        }
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        if (!opt_recursive) {
            fprintf(stderr, "rm: cannot remove '%s': Is a directory (use -r)\n", path);
            return -1;
        }
        return remove_dir(path);
    } else {
        if (unlink(path) < 0) {
            if (!opt_force) {
                fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
            }
            return -1;
        }

        if (verbose >= 1) {
            printf("removed '%s'\n", path);
        }
        return 0;
    }
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
            printf("rm version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "-R") == 0 ||
                   strcmp(argv[i], "--recursive") == 0) {
            opt_recursive = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            opt_force = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "rm: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        if (!opt_force) {
            fprintf(stderr, "rm: missing operand\n");
            print_usage(argv[0]);
            return 1;
        }
        return 0;
    }

    int ret = 0;
    for (int i = 0; i < file_count; i++) {
        if (remove_recursive(files[i]) < 0) {
            ret = 1;
        }
    }

    return ret;
}
