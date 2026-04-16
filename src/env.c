/*
 * env - display or set environment variables for command execution
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

extern char **environ;
static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [NAME=VALUE]... [COMMAND [ARG...]]\n", progname);
    fprintf(stderr, "Set each NAME to VALUE in the environment and run COMMAND.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -i, --ignore-environment  Start with an empty environment\n");
    fprintf(stderr, "  -u, --unset=NAME          Remove variable from the environment\n");
    fprintf(stderr, "  -v, --verbose             Verbose output\n");
    fprintf(stderr, "  --version                 Print version\n");
    fprintf(stderr, "  -h, --help                Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int ignore_env = 0;
    char *unset_vars[128];
    int unset_count = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("env version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--ignore-environment") == 0) {
            ignore_env = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strncmp(argv[i], "-u", 2) == 0) {
            if (argv[i][2] == '=') {
                /* -u=VAR */
                if (unset_count < 128) unset_vars[unset_count++] = argv[i] + 3;
            } else if (argv[i][2] == '\0') {
                /* -u VAR */
                if (++i < argc && unset_count < 128) {
                    unset_vars[unset_count++] = argv[i];
                }
            }
        } else if (strncmp(argv[i], "--unset=", 8) == 0) {
            if (unset_count < 128) unset_vars[unset_count++] = argv[i] + 8;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "env: invalid option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            break;
        }
    }

    /* Clear environment if requested */
    if (ignore_env) {
        clearenv();
    }

    /* Unset variables */
    for (int j = 0; j < unset_count; j++) {
        unsetenv(unset_vars[j]);
    }

    /* Process NAME=VALUE pairs and find command */
    int cmd_idx = i;
    for (; i < argc; i++) {
        if (strchr(argv[i], '=') != NULL) {
            char *name = strdup(argv[i]);
            if (name == NULL) {
                fprintf(stderr, "env: failed to duplicate '%s': %s\n", argv[i], strerror(errno));
                return 1;
            }
            char *value = strchr(name, '=');
            *value = '\0';
            value++;
            if (setenv(name, value, 1) < 0) {
                fprintf(stderr, "env: failed to set '%s': %s\n", name, strerror(errno));
                free(name);
                return 1;
            }
            if (verbose >= 1) {
                fprintf(stderr, "env: set %s=%s\n", name, value);
            }
            free(name);
            cmd_idx = i + 1;
        } else {
            break;
        }
    }

    /* If no command, print environment */
    if (cmd_idx >= argc) {
        for (char **env = environ; *env != NULL; env++) {
            printf("%s\n", *env);
        }
        return 0;
    }

    /* Execute command */
    if (verbose >= 1) {
        fprintf(stderr, "env: executing '%s'\n", argv[cmd_idx]);
    }

    execvp(argv[cmd_idx], &argv[cmd_idx]);
    fprintf(stderr, "env: failed to execute '%s': %s\n", argv[cmd_idx], strerror(errno));
    return 127;
}
