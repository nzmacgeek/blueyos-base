/*
 * stat - display file or file system status
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] FILE...\n", progname);
    fprintf(stderr, "Display file or file system status.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -L, --dereference  Follow links\n");
    fprintf(stderr, "  -v, --verbose      Verbose output\n");
    fprintf(stderr, "  --version          Print version\n");
    fprintf(stderr, "  -h, --help         Show this help\n");
}

static const char *file_type_str(mode_t mode) {
    if (S_ISREG(mode))  return "regular file";
    if (S_ISDIR(mode))  return "directory";
    if (S_ISCHR(mode))  return "character device";
    if (S_ISBLK(mode))  return "block device";
    if (S_ISFIFO(mode)) return "FIFO/pipe";
    if (S_ISLNK(mode))  return "symbolic link";
    if (S_ISSOCK(mode)) return "socket";
    return "unknown";
}

static void print_perms(mode_t mode) {
    printf("(0%03o/", mode & 0777);
    printf("%c%c%c%c%c%c%c%c%c)",
           (mode & S_IRUSR) ? 'r' : '-',
           (mode & S_IWUSR) ? 'w' : '-',
           (mode & S_IXUSR) ? 'x' : '-',
           (mode & S_IRGRP) ? 'r' : '-',
           (mode & S_IWGRP) ? 'w' : '-',
           (mode & S_IXGRP) ? 'x' : '-',
           (mode & S_IROTH) ? 'r' : '-',
           (mode & S_IWOTH) ? 'w' : '-',
           (mode & S_IXOTH) ? 'x' : '-');
}

static int stat_file(const char *path, int dereference) {
    struct stat st;
    int ret;

    if (dereference) {
        ret = stat(path, &st);
    } else {
        ret = lstat(path, &st);
    }

    if (ret < 0) {
        fprintf(stderr, "stat: cannot stat '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("  File: %s\n", path);
    printf("  Size: %-15lld Blocks: %-10lld IO Block: %ld\n",
           (long long)st.st_size,
           (long long)st.st_blocks,
           (long)st.st_blksize);

    printf("Device: %llxh/%llud  Inode: %-10llu Links: %lu\n",
           (unsigned long long)st.st_dev,
           (unsigned long long)st.st_dev,
           (unsigned long long)st.st_ino,
           (unsigned long)st.st_nlink);

    printf("Access: ");
    print_perms(st.st_mode);
    printf("  Uid: (%5u/", (unsigned)st.st_uid);
    struct passwd *pw = getpwuid(st.st_uid);
    if (pw) printf("%8s)", pw->pw_name);
    else    printf("%8s)", "UNKNOWN");
    printf("   Gid: (%5u/", (unsigned)st.st_gid);
    struct group *gr = getgrgid(st.st_gid);
    if (gr) printf("%8s)\n", gr->gr_name);
    else    printf("%8s)\n", "UNKNOWN");

    char timebuf[64];
    struct tm *tm;

    printf("Access: ");
    tm = localtime(&st.st_atime);
    if (tm) {
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);
        printf("%s\n", timebuf);
    } else {
        printf("?\n");
    }

    printf("Modify: ");
    tm = localtime(&st.st_mtime);
    if (tm) {
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);
        printf("%s\n", timebuf);
    } else {
        printf("?\n");
    }

    printf("Change: ");
    tm = localtime(&st.st_ctime);
    if (tm) {
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);
        printf("%s\n", timebuf);
    } else {
        printf("?\n");
    }

    printf(" Birth: -\n");

    return 0;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int opt_dereference = 0;
    char *files[256];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("stat version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-L") == 0 || strcmp(argv[i], "--dereference") == 0) {
            opt_dereference = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "stat: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "stat: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    int ret = 0;
    for (int i = 0; i < file_count; i++) {
        if (stat_file(files[i], opt_dereference) < 0) {
            ret = 1;
        }
        if (i < file_count - 1) printf("\n");
    }

    return ret;
}
