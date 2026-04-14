/*
 * uname - print system information
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a    Print all information\n");
    fprintf(stderr, "  -s    Print kernel name\n");
    fprintf(stderr, "  -n    Print node name\n");
    fprintf(stderr, "  -r    Print kernel release\n");
    fprintf(stderr, "  -v    Print kernel version\n");
    fprintf(stderr, "  -m    Print machine hardware\n");
    fprintf(stderr, "  -p    Print processor type (unknown)\n");
    fprintf(stderr, "  -i    Print hardware platform (unknown)\n");
    fprintf(stderr, "  --verbose    Verbose output\n");
    fprintf(stderr, "  --version    Print version\n");
    fprintf(stderr, "  -h, --help   Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int opt_a = 0, opt_s = 0, opt_n = 0, opt_r = 0;
    int opt_v = 0, opt_m = 0, opt_p = 0, opt_i = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("uname version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-' && argv[i][1] != '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'a': opt_a = 1; break;
                    case 's': opt_s = 1; break;
                    case 'n': opt_n = 1; break;
                    case 'r': opt_r = 1; break;
                    case 'v': opt_v = 1; break;
                    case 'm': opt_m = 1; break;
                    case 'p': opt_p = 1; break;
                    case 'i': opt_i = 1; break;
                    default:
                        fprintf(stderr, "uname: invalid option -- '%c'\n", argv[i][j]);
                        return 1;
                }
            }
        } else {
            fprintf(stderr, "uname: unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    struct utsname uts;
    if (uname(&uts) < 0) {
        perror("uname");
        return 1;
    }

    if (opt_a) {
        opt_s = opt_n = opt_r = opt_v = opt_m = opt_p = opt_i = 1;
    }

    /* Default: nodename */
    if (!opt_s && !opt_n && !opt_r && !opt_v && !opt_m && !opt_p && !opt_i) {
        opt_n = 1;
    }

    int first = 1;
    if (opt_s) { if (!first) printf(" "); printf("%s", uts.sysname);  first = 0; }
    if (opt_n) { if (!first) printf(" "); printf("%s", uts.nodename); first = 0; }
    if (opt_r) { if (!first) printf(" "); printf("%s", uts.release);  first = 0; }
    if (opt_v) { if (!first) printf(" "); printf("%s", uts.version);  first = 0; }
    if (opt_m) { if (!first) printf(" "); printf("%s", uts.machine);  first = 0; }
    if (opt_p) { if (!first) printf(" "); printf("unknown");           first = 0; }
    if (opt_i) { if (!first) printf(" "); printf("unknown");           first = 0; }
    printf("\n");

    return 0;
}
