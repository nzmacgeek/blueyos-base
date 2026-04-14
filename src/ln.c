/*
 * ln - create links between files
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

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] TARGET [LINK_NAME]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s    Make symbolic links instead of hard links\n");
    fprintf(stderr, "  -f    Remove existing destination files\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("ln version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    int opt_s = 0, opt_f = 0;
    char *target = NULL;
    char *link_name = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 's': opt_s = 1; break;
                    case 'f': opt_f = 1; break;
                    case 'v': verbose = 1; break;
                    default:
                        fprintf(stderr, "ln: invalid option -- '%c'\n", argv[i][j]);
                        return 1;
                }
            }
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "ln: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (!target) {
            target = argv[i];
        } else if (!link_name) {
            link_name = argv[i];
        } else {
            fprintf(stderr, "ln: too many arguments\n");
            return 1;
        }
    }

    if (!target) {
        fprintf(stderr, "ln: missing file operand\n");
        print_usage(argv[0]);
        return 1;
    }

    /* If no link name given, use basename of target in current directory */
    char default_link[256];
    if (!link_name) {
        const char *base = strrchr(target, '/');
        base = base ? base + 1 : target;
        strncpy(default_link, base, sizeof(default_link) - 1);
        default_link[sizeof(default_link) - 1] = '\0';
        link_name = default_link;
    }

    if (opt_f) {
        unlink(link_name);
    }

    int ret;
    if (opt_s) {
        ret = symlink(target, link_name);
    } else {
        ret = link(target, link_name);
    }

    if (ret < 0) {
        fprintf(stderr, "ln: failed to create %slink '%s' -> '%s': %s\n",
                opt_s ? "symbolic " : "", link_name, target, strerror(errno));
        return 1;
    }

    if (verbose >= 1) {
        printf("'%s' -> '%s'\n", link_name, target);
    }

    return 0;
}
