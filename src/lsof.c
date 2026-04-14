/*
 * lsof - list open files
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
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

static const char *fd_type(struct stat *st) {
    if (S_ISREG(st->st_mode))  return "REG";
    if (S_ISDIR(st->st_mode))  return "DIR";
    if (S_ISCHR(st->st_mode))  return "CHR";
    if (S_ISBLK(st->st_mode))  return "BLK";
    if (S_ISFIFO(st->st_mode)) return "FIFO";
    if (S_ISSOCK(st->st_mode)) return "SOCK";
    if (S_ISLNK(st->st_mode))  return "LNK";
    return "UNKN";
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("lsof version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else {
            fprintf(stderr, "lsof: unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    printf("%-16s %6s %4s %-4s %s\n", "COMMAND", "PID", "FD", "TYPE", "NAME");

    DIR *proc = opendir("/proc");
    if (!proc) {
        fprintf(stderr, "lsof: cannot open /proc: %s\n", strerror(errno));
        return 1;
    }

    struct dirent *pent;
    while ((pent = readdir(proc)) != NULL) {
        if (!is_pid_dir(pent->d_name)) continue;

        char comm[256] = "";
        char comm_path[256];
        snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", pent->d_name);
        FILE *fp = fopen(comm_path, "r");
        if (fp) {
            if (fgets(comm, sizeof(comm), fp)) {
                size_t l = strlen(comm);
                if (l > 0 && comm[l-1] == '\n') comm[l-1] = '\0';
            }
            fclose(fp);
        }
        if (comm[0] == '\0') snprintf(comm, sizeof(comm), "<%s>", pent->d_name);

        char fd_dir[256];
        snprintf(fd_dir, sizeof(fd_dir), "/proc/%s/fd", pent->d_name);

        DIR *fds = opendir(fd_dir);
        if (!fds) continue; /* skip unreadable entries */

        struct dirent *fent;
        while ((fent = readdir(fds)) != NULL) {
            if (fent->d_name[0] == '.') continue;

            char fd_path[512];
            snprintf(fd_path, sizeof(fd_path), "%s/%s", fd_dir, fent->d_name);

            char target[512] = "";
            ssize_t n = readlink(fd_path, target, sizeof(target) - 1);
            if (n < 0) continue;
            target[n] = '\0';

            struct stat st;
            const char *type = "UNKN";
            if (stat(fd_path, &st) == 0) {
                type = fd_type(&st);
            }

            printf("%-16s %6s %4s %-4s %s\n",
                   comm, pent->d_name, fent->d_name, type, target);
        }
        closedir(fds);
    }
    closedir(proc);
    return 0;
}
