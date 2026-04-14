/*
 * ps - report process status
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
    fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
    fprintf(stderr, "Options:\n");
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
            printf("ps version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else {
            fprintf(stderr, "ps: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    DIR *proc = opendir("/proc");
    if (!proc) {
        fprintf(stderr, "ps: cannot open /proc: %s\n", strerror(errno));
        return 1;
    }

    printf("  PID TTY          TIME CMD\n");

    struct dirent *ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!is_pid_dir(ent->d_name)) continue;

        char path[256];
        char line[1024];
        char comm[256] = "?";
        char state[8] = "?";
        char tty[32] = "?";
        char cmdline[1024] = "";

        /* Read /proc/PID/status */
        snprintf(path, sizeof(path), "/proc/%s/status", ent->d_name);
        FILE *fp = fopen(path, "r");
        if (!fp) continue; /* skip unreadable (permission denied) */

        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "Name:\t", 6) == 0) {
                strncpy(comm, line + 6, sizeof(comm) - 1);
                comm[sizeof(comm) - 1] = '\0';
                /* strip trailing newline */
                size_t l = strlen(comm);
                if (l > 0 && comm[l-1] == '\n') comm[l-1] = '\0';
            } else if (strncmp(line, "State:\t", 7) == 0) {
                state[0] = line[7];
                state[1] = '\0';
            }
        }
        fclose(fp);

        /* Read /proc/PID/cmdline */
        snprintf(path, sizeof(path), "/proc/%s/cmdline", ent->d_name);
        fp = fopen(path, "r");
        if (fp) {
            size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
            fclose(fp);
            cmdline[n] = '\0';
            /* Replace null bytes with spaces */
            for (size_t j = 0; j < n; j++) {
                if (cmdline[j] == '\0') cmdline[j] = ' ';
            }
            /* Trim trailing space */
            size_t l = strlen(cmdline);
            while (l > 0 && cmdline[l-1] == ' ') cmdline[--l] = '\0';
        }

        const char *cmd_display = (cmdline[0] != '\0') ? cmdline : comm;
        printf("%5s %-12s %5s %s\n", ent->d_name, tty, state, cmd_display);
    }

    closedir(proc);
    return 0;
}
