/*
 * gpasswd - administer /etc/group
 * Part of BlueyOS base utilities
 *
 * A simplified tool for adding/removing users to/from groups
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] GROUP\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a, --add USER        Add USER to GROUP\n");
    fprintf(stderr, "  -d, --delete USER     Remove USER from GROUP\n");
    fprintf(stderr, "  -v, --verbose         Verbose output\n");
    fprintf(stderr, "  --version             Print version\n");
    fprintf(stderr, "  -h, --help            Show this help\n");
}

int main(int argc, char *argv[]) {
    int i;
    char *group = NULL;
    char *user = NULL;
    int add_user = 0;
    int del_user = 0;
    int verbose = 0;
    char *usermod_args[8];
    int arg_count = 0;

    /* Check for VERBOSE environment variable */
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Parse arguments - check for version/help first before requiring root */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("gpasswd version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    /* Check if running as root */
    if (getuid() != 0 && geteuid() != 0) {
        fprintf(stderr, "gpasswd: permission denied (must be root)\n");
        return 1;
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--add") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "gpasswd: -a requires a username\n");
                return 1;
            }
            add_user = 1;
            user = argv[i];
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--delete") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "gpasswd: -d requires a username\n");
                return 1;
            }
            del_user = 1;
            user = argv[i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "gpasswd: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            /* It's the group */
            group = argv[i];
        }
    }

    if (!group) {
        fprintf(stderr, "gpasswd: no group specified\n");
        print_usage(argv[0]);
        return 1;
    }

    if (!add_user && !del_user) {
        fprintf(stderr, "gpasswd: no operation specified (use -a or -d)\n");
        print_usage(argv[0]);
        return 1;
    }

    if (add_user && del_user) {
        fprintf(stderr, "gpasswd: cannot use -a and -d together\n");
        return 1;
    }

    if (!user) {
        fprintf(stderr, "gpasswd: no user specified\n");
        return 1;
    }

    /* Build usermod command */
    usermod_args[arg_count++] = "usermod";
    if (verbose) {
        usermod_args[arg_count++] = "-v";
    }
    if (add_user) {
        usermod_args[arg_count++] = "-aG";
    } else {
        usermod_args[arg_count++] = "-rG";
    }
    usermod_args[arg_count++] = group;
    usermod_args[arg_count++] = user;
    usermod_args[arg_count] = NULL;

    /* Execute usermod */
    execvp("usermod", usermod_args);

    /* If we get here, exec failed */
    fprintf(stderr, "gpasswd: failed to execute usermod: %s\n", strerror(errno));
    return 1;
}
