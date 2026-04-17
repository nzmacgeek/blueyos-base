/*
 * halt - halt the system
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
    fprintf(stderr, "Halt the system.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -p, --poweroff   Power off after halt\n");
    fprintf(stderr, "  -f, --force      Force immediate halt (skip sync)\n");
    fprintf(stderr, "  -n, --no-sync    Don't sync before halt\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int do_sync = 1;
    int cmd = RB_HALT_SYSTEM;  /* Default: halt without poweroff */

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("halt version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--poweroff") == 0) {
            cmd = RB_POWER_OFF;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            do_sync = 0;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-sync") == 0) {
            do_sync = 0;
        } else {
            fprintf(stderr, "halt: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Check if we're running as root (real or effective UID) */
    if (getuid() != 0 && geteuid() != 0) {
        fprintf(stderr, "halt: must be run as root\n");
        return 1;
    }

    /* Sync filesystems unless told not to */
    if (do_sync) {
        if (verbose >= 1) {
            fprintf(stderr, "halt: syncing filesystems...\n");
        }
        sync();
    }

    const char *action_name = (cmd == RB_POWER_OFF) ? "powering off" : "halting";
    if (verbose >= 1) {
        fprintf(stderr, "halt: %s system...\n", action_name);
    }

    /* Call the reboot syscall */
    if (reboot(cmd) < 0) {
        fprintf(stderr, "halt: reboot syscall failed: %s\n", strerror(errno));
        return 1;
    }

    /* Should not reach here */
    return 0;
}
