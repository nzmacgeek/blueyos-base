/*
 * kill - send a signal to a process
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [-SIGNAL] PID...\n", progname);
    fprintf(stderr, "       %s -l\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -SIGNAL          Signal name or number (default: TERM)\n");
    fprintf(stderr, "  -l               List signal names\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int parse_signal(const char *s) {
    if (isdigit((unsigned char)s[0])) {
        return atoi(s);
    }
    /* Strip SIG prefix */
    const char *name = s;
    if (strncasecmp(s, "SIG", 3) == 0) name = s + 3;

    if (strcasecmp(name, "HUP") == 0)  return SIGHUP;
    if (strcasecmp(name, "INT") == 0)  return SIGINT;
    if (strcasecmp(name, "QUIT") == 0) return SIGQUIT;
    if (strcasecmp(name, "KILL") == 0) return SIGKILL;
    if (strcasecmp(name, "TERM") == 0) return SIGTERM;
    if (strcasecmp(name, "STOP") == 0) return SIGSTOP;
    if (strcasecmp(name, "CONT") == 0) return SIGCONT;
    if (strcasecmp(name, "USR1") == 0) return SIGUSR1;
    if (strcasecmp(name, "USR2") == 0) return SIGUSR2;
    return -1;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("kill version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    int sig = SIGTERM;
    int list_mode = 0;
    int pids_start = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            list_mode = 1;
            pids_start = i + 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
            pids_start = i + 1;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            /* -SIGNAL or -N */
            int s = parse_signal(argv[i] + 1);
            if (s < 0) {
                fprintf(stderr, "kill: invalid signal '%s'\n", argv[i] + 1);
                return 1;
            }
            sig = s;
            pids_start = i + 1;
        } else {
            pids_start = i;
            break;
        }
    }

    if (list_mode) {
        printf("HUP INT QUIT ILL TRAP ABRT BUS FPE KILL USR1 SEGV USR2 PIPE ALRM TERM\n");
        return 0;
    }

    if (pids_start >= argc) {
        fprintf(stderr, "kill: no PIDs specified\n");
        print_usage(argv[0]);
        return 1;
    }

    int ret = 0;
    for (int i = pids_start; i < argc; i++) {
        pid_t pid = (pid_t)atoi(argv[i]);
        if (pid == 0 && argv[i][0] != '0') {
            fprintf(stderr, "kill: invalid PID '%s'\n", argv[i]);
            ret = 1;
            continue;
        }
        if (kill(pid, sig) < 0) {
            fprintf(stderr, "kill: (%d) - %s\n", (int)pid, strerror(errno));
            ret = 1;
        } else if (verbose >= 1) {
            fprintf(stderr, "kill: sent signal %d to PID %d\n", sig, (int)pid);
        }
    }

    return ret;
}
