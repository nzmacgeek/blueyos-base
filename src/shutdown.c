/*
 * shutdown - bring the system down
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/reboot.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
    fprintf(stderr, "Bring the system down.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --halt       Halt the system (don't power off)\n");
    fprintf(stderr, "  -P, --poweroff   Power off the system (default)\n");
    fprintf(stderr, "  -r, --reboot     Reboot the system\n");
    fprintf(stderr, "  -n, --no-sync    Don't sync before shutdown\n");
    fprintf(stderr, "  -f, --force      Force immediate shutdown (skip sync)\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  --help           Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int do_sync = 1;
    int cmd = RB_POWER_OFF;  /* Default action */

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("shutdown version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--halt") == 0) {
            cmd = RB_HALT_SYSTEM;
        } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--poweroff") == 0) {
            cmd = RB_POWER_OFF;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reboot") == 0) {
            cmd = RB_AUTOBOOT;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-sync") == 0) {
            do_sync = 0;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            do_sync = 0;
        } else {
            fprintf(stderr, "shutdown: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Check if we're running as root (real or effective UID) */
    if (getuid() != 0 && geteuid() != 0) {
        fprintf(stderr, "shutdown: must be run as root\n");
        return 1;
    }

    /* Sync filesystems unless told not to */
    if (do_sync) {
        if (verbose >= 1) {
            fprintf(stderr, "shutdown: syncing filesystems...\n");
        }
        sync();
    }

    /* Perform the shutdown action */
    const char *action_name = "unknown";
    switch (cmd) {
        case RB_HALT_SYSTEM:
            action_name = "halting";
            break;
        case RB_POWER_OFF:
            action_name = "powering off";
            break;
        case RB_AUTOBOOT:
            action_name = "rebooting";
            break;
    }

    if (verbose >= 1) {
        fprintf(stderr, "shutdown: %s system...\n", action_name);
    }

    /* Call the reboot syscall */
    if (reboot(cmd) < 0) {
        fprintf(stderr, "shutdown: reboot syscall failed: %s\n", strerror(errno));
        return 1;
    }

    /* Should not reach here */
    return 0;
}
