/*
 * du - estimate file space usage
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_summary = 0;
static int opt_human = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s    Display only a total for each argument\n");
    fprintf(stderr, "  -h    Human readable sizes\n");
    fprintf(stderr, "  -k    Use 1K blocks (default)\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  --help           Show this help\n");
}

static void human_size(unsigned long long kb, char *buf, size_t buflen) {
    if (kb >= 1024 * 1024) {
        snprintf(buf, buflen, "%.1fG", (double)kb / (1024.0 * 1024.0));
    } else if (kb >= 1024) {
        snprintf(buf, buflen, "%.1fM", (double)kb / 1024.0);
    } else {
        snprintf(buf, buflen, "%lluK", kb);
    }
}

/* Returns disk usage in 1K blocks */
static unsigned long long du_path(const char *path, int depth) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        if (verbose >= 1) {
            fprintf(stderr, "du: cannot stat '%s': %s\n", path, strerror(errno));
        }
        return 0;
    }

    /* st_blocks is in 512-byte units; divide by 2 for 1K */
    unsigned long long usage = (unsigned long long)st.st_blocks / 2;

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            if (verbose >= 1) {
                fprintf(stderr, "du: cannot open '%s': %s\n", path, strerror(errno));
            }
            return usage;
        }
        struct dirent *ent;
        char child[4096];
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
            unsigned long long child_usage = du_path(child, depth + 1);
            usage += child_usage;
            if (!opt_summary && depth >= 0) {
                /* Print each sub-entry */
                struct stat cst;
                if (lstat(child, &cst) == 0 && S_ISDIR(cst.st_mode)) {
                    if (opt_human) {
                        char buf[32];
                        human_size(child_usage, buf, sizeof(buf));
                        printf("%s\t%s\n", buf, child);
                    } else {
                        printf("%llu\t%s\n", child_usage, child);
                    }
                }
            }
        }
        closedir(dir);
    }

    return usage;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *paths[256];
    int path_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("du version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            if (strcmp(argv[i], "-h") == 0 && i + 1 < argc && argv[i+1][0] != '-') {
                opt_human = 1; /* -h as human-readable, not help */
            } else if (strcmp(argv[i], "--help") == 0) {
                print_usage(argv[0]);
                return 0;
            } else {
                opt_human = 1;
            }
        } else if (strcmp(argv[i], "-s") == 0) {
            opt_summary = 1;
        } else if (strcmp(argv[i], "-k") == 0) {
            /* 1K blocks is default, no-op */
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            /* Handle combined flags like -sh */
            for (int j = 1; argv[i][j]; j++) {
                if (argv[i][j] == 's') opt_summary = 1;
                else if (argv[i][j] == 'h') opt_human = 1;
                else if (argv[i][j] == 'k') { /* no-op */ }
                else {
                    fprintf(stderr, "du: unknown option '-%c'\n", argv[i][j]);
                    return 1;
                }
            }
        } else {
            if (path_count < 256) paths[path_count++] = argv[i];
        }
    }

    if (path_count == 0) {
        paths[path_count++] = ".";
    }

    for (int i = 0; i < path_count; i++) {
        unsigned long long usage = du_path(paths[i], 0);
        if (opt_human) {
            char buf[32];
            human_size(usage, buf, sizeof(buf));
            printf("%s\t%s\n", buf, paths[i]);
        } else {
            printf("%llu\t%s\n", usage, paths[i]);
        }
    }

    return 0;
}
