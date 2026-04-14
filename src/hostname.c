/*
 * hostname - get or set the system hostname
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#define HOSTNAME_FILE "/etc/hostname"
#define HOSTNAME_MAX 256

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [HOSTNAME]\n", progname);
    fprintf(stderr, "Options:\n");
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
            printf("hostname version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    char *new_hostname = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "hostname: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            new_hostname = argv[i];
        }
    }

    if (new_hostname) {
        /* Setting hostname requires root */
        if (getuid() != 0 && geteuid() != 0) {
            fprintf(stderr, "hostname: permission denied\n");
            return 1;
        }

        if (sethostname(new_hostname, strlen(new_hostname)) < 0) {
            fprintf(stderr, "hostname: sethostname failed: %s\n", strerror(errno));
            return 1;
        }

        /* Also update /etc/hostname if it exists */
        FILE *fp = fopen(HOSTNAME_FILE, "w");
        if (fp) {
            fprintf(fp, "%s\n", new_hostname);
            fclose(fp);
            if (verbose >= 1) {
                fprintf(stderr, "hostname: updated %s\n", HOSTNAME_FILE);
            }
        }

        if (verbose >= 1) {
            printf("Hostname set to '%s'\n", new_hostname);
        }
    } else {
        /* Get hostname */
        char buf[HOSTNAME_MAX];
        if (gethostname(buf, sizeof(buf)) < 0) {
            fprintf(stderr, "hostname: gethostname failed: %s\n", strerror(errno));
            return 1;
        }
        buf[sizeof(buf) - 1] = '\0';
        printf("%s\n", buf);
    }

    return 0;
}
