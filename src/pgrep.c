/*
 * pgrep - look up processes by name
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] PATTERN\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -l    Print process name along with PID\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int is_pid_dir(const char *name) {
    for (int i = 0; name[i]; i++) {
        if (!isdigit((unsigned char)name[i])) return 0;
    }
    return name[0] != '\0';
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("pgrep version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    int opt_l = 0;
    char *pattern = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            opt_l = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "pgrep: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            pattern = argv[i];
        }
    }

    if (!pattern) {
        fprintf(stderr, "pgrep: no pattern specified\n");
        print_usage(argv[0]);
        return 1;
    }

    DIR *proc = opendir("/proc");
    if (!proc) {
        fprintf(stderr, "pgrep: cannot open /proc: %s\n", strerror(errno));
        return 1;
    }

    int found = 0;
    struct dirent *ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!is_pid_dir(ent->d_name)) continue;

        char path[256];
        char comm[256] = "";

        /* Try /proc/PID/comm first */
        snprintf(path, sizeof(path), "/proc/%s/comm", ent->d_name);
        FILE *fp = fopen(path, "r");
        if (fp) {
            if (fgets(comm, sizeof(comm), fp)) {
                /* strip newline */
                size_t l = strlen(comm);
                if (l > 0 && comm[l-1] == '\n') comm[l-1] = '\0';
            }
            fclose(fp);
        }

        if (comm[0] == '\0') {
            /* Fallback to Name: in status */
            snprintf(path, sizeof(path), "/proc/%s/status", ent->d_name);
            fp = fopen(path, "r");
            if (fp) {
                char line[256];
                while (fgets(line, sizeof(line), fp)) {
                    if (strncmp(line, "Name:\t", 6) == 0) {
                        strncpy(comm, line + 6, sizeof(comm) - 1);
                        comm[sizeof(comm) - 1] = '\0';
                        size_t l = strlen(comm);
                        if (l > 0 && comm[l-1] == '\n') comm[l-1] = '\0';
                        break;
                    }
                }
                fclose(fp);
            }
        }

        if (comm[0] == '\0') continue;

        /* Match pattern against process name */
        if (strstr(comm, pattern) != NULL) {
            found++;
            if (opt_l) {
                printf("%s %s\n", ent->d_name, comm);
            } else {
                printf("%s\n", ent->d_name);
            }
        }
    }
    closedir(proc);

    return (found > 0) ? 0 : 1;
}
