/*
 * which - locate a command
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
static int opt_all = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] COMMAND...\n", progname);
    fprintf(stderr, "Locate a command in PATH.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a, --all        Print all matching pathnames\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int which_command(const char *cmd) {
    /* If cmd contains '/', test it directly without PATH search */
    if (strchr(cmd, '/') != NULL) {
        struct stat st;
        if (stat(cmd, &st) == 0 && S_ISREG(st.st_mode) && access(cmd, X_OK) == 0) {
            printf("%s\n", cmd);
            return 0;
        }
        return -1;
    }

    const char *path_env = getenv("PATH");
    if (!path_env) {
        path_env = "/usr/local/bin:/usr/bin:/bin";
    }

    char *path = strdup(path_env);
    if (!path) return -1;

    int found = 0;
    char *dir = strtok(path, ":");
    while (dir) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode) && access(full_path, X_OK) == 0) {
            printf("%s\n", full_path);
            found = 1;
            if (!opt_all) break;
        }

        dir = strtok(NULL, ":");
    }

    free(path);
    return found ? 0 : -1;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *commands[256];
    int cmd_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("which version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            opt_all = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "which: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (cmd_count < 256) commands[cmd_count++] = argv[i];
        }
    }

    if (cmd_count == 0) {
        fprintf(stderr, "which: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    int ret = 0;
    for (int i = 0; i < cmd_count; i++) {
        if (which_command(commands[i]) < 0) {
            ret = 1;
        }
    }

    return ret;
}
