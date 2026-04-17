/*
 * poweroff - power off the system
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
    fprintf(stderr, "Power off the system.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -f, --force      Force immediate poweroff (skip sync)\n");
    fprintf(stderr, "  -n, --no-sync    Don't sync before poweroff\n");
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

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("poweroff version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            do_sync = 0;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-sync") == 0) {
            do_sync = 0;
        } else {
            fprintf(stderr, "poweroff: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Check if we're running as root */
    if (getuid() != 0) {
        fprintf(stderr, "poweroff: must be run as root\n");
        return 1;
    }

    /* Sync filesystems unless told not to */
    if (do_sync) {
        if (verbose >= 1) {
            fprintf(stderr, "poweroff: syncing filesystems...\n");
        }
        sync();
    }

    if (verbose >= 1) {
        fprintf(stderr, "poweroff: powering off system...\n");
    }

    /* Call the reboot syscall */
    if (reboot(RB_POWER_OFF) < 0) {
        fprintf(stderr, "poweroff: reboot syscall failed: %s\n", strerror(errno));
        return 1;
    }

    /* Should not reach here */
    return 0;
}
