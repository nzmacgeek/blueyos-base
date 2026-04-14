/*
 * df - report filesystem disk space usage
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static void print_fs(const char *dev, const char *mnt) {
    struct statvfs st;
    if (statvfs(mnt, &st) < 0) {
        if (verbose >= 1) {
            fprintf(stderr, "df: cannot statvfs '%s': %s\n", mnt, strerror(errno));
        }
        return;
    }

    unsigned long long block_size = st.f_frsize ? st.f_frsize : st.f_bsize;
    unsigned long long total_1k = (st.f_blocks * block_size) / 1024;
    unsigned long long avail_1k = (st.f_bavail * block_size) / 1024;
    unsigned long long used_1k = total_1k > avail_1k ? total_1k - avail_1k : 0;
    int use_pct = (total_1k > 0) ? (int)((used_1k * 100) / total_1k) : 0;

    printf("%-20s %10llu %10llu %10llu %3d%% %s\n",
           dev, total_1k, used_1k, avail_1k, use_pct, mnt);
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("df version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "df: unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    printf("%-20s %10s %10s %10s %4s %s\n",
           "Filesystem", "1K-blocks", "Used", "Available", "Use%", "Mounted on");

    FILE *mounts = fopen("/proc/mounts", "r");
    if (!mounts) {
        fprintf(stderr, "df: cannot open /proc/mounts: %s\n", strerror(errno));
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), mounts)) {
        char dev[256], mnt[256], fstype[64];
        if (sscanf(line, "%255s %255s %63s", dev, mnt, fstype) < 3) continue;
        /* Skip pseudo filesystems */
        if (strcmp(fstype, "proc") == 0 || strcmp(fstype, "sysfs") == 0 ||
            strcmp(fstype, "devtmpfs") == 0 || strcmp(fstype, "devpts") == 0 ||
            strcmp(fstype, "tmpfs") == 0 || strcmp(fstype, "cgroup") == 0 ||
            strcmp(fstype, "cgroup2") == 0 || strcmp(fstype, "pstore") == 0 ||
            strcmp(fstype, "securityfs") == 0 || strcmp(fstype, "debugfs") == 0 ||
            strcmp(fstype, "hugetlbfs") == 0 || strcmp(fstype, "mqueue") == 0 ||
            strcmp(fstype, "binfmt_misc") == 0) {
            continue;
        }
        print_fs(dev, mnt);
    }
    fclose(mounts);
    return 0;
}
