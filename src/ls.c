/*
 * ls - list directory contents
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_long = 0;
static int opt_all = 0;
static int opt_human = 0;
static int opt_reverse = 0;
static int opt_time_sort = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -l    Long listing format\n");
    fprintf(stderr, "  -a    Include hidden files (starting with .)\n");
    fprintf(stderr, "  -1    One entry per line (default)\n");
    fprintf(stderr, "  -h    Human readable sizes (with -l)\n");
    fprintf(stderr, "  -r    Reverse sort order\n");
    fprintf(stderr, "  -t    Sort by modification time\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  --help           Show this help\n");
}

static void human_size(off_t size, char *buf, size_t buflen) {
    if (size >= 1024LL * 1024 * 1024) {
        snprintf(buf, buflen, "%.1fG", (double)size / (1024.0 * 1024.0 * 1024.0));
    } else if (size >= 1024 * 1024) {
        snprintf(buf, buflen, "%.1fM", (double)size / (1024.0 * 1024.0));
    } else if (size >= 1024) {
        snprintf(buf, buflen, "%.1fK", (double)size / 1024.0);
    } else {
        snprintf(buf, buflen, "%lld", (long long)size);
    }
}

static void print_perms(mode_t mode) {
    char type;
    if (S_ISDIR(mode)) type = 'd';
    else if (S_ISLNK(mode)) type = 'l';
    else if (S_ISBLK(mode)) type = 'b';
    else if (S_ISCHR(mode)) type = 'c';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISSOCK(mode)) type = 's';
    else type = '-';

    printf("%c%c%c%c%c%c%c%c%c%c",
           type,
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

typedef struct {
    char name[512];
    struct stat st;
} entry_t;

static int cmp_name(const void *a, const void *b) {
    const entry_t *ea = (const entry_t *)a;
    const entry_t *eb = (const entry_t *)b;
    return strcmp(ea->name, eb->name);
}

static int cmp_mtime(const void *a, const void *b) {
    const entry_t *ea = (const entry_t *)a;
    const entry_t *eb = (const entry_t *)b;
    if (eb->st.st_mtime > ea->st.st_mtime) return 1;
    if (eb->st.st_mtime < ea->st.st_mtime) return -1;
    return 0;
}

static void print_entry(const char *path, const char *name, struct stat *st) {
    if (!opt_long) {
        printf("%s\n", name);
        return;
    }

    /* Long format */
    print_perms(st->st_mode);

    /* Link count, owner, group */
    struct passwd *pw = getpwuid(st->st_uid);
    struct group *gr = getgrgid(st->st_gid);

    char owner[32], group[32];
    if (pw) snprintf(owner, sizeof(owner), "%s", pw->pw_name);
    else    snprintf(owner, sizeof(owner), "%u", (unsigned)st->st_uid);
    if (gr) snprintf(group, sizeof(group), "%s", gr->gr_name);
    else    snprintf(group, sizeof(group), "%u", (unsigned)st->st_gid);

    printf(" %3lu %-8s %-8s ", (unsigned long)st->st_nlink, owner, group);

    if (opt_human) {
        char sbuf[32];
        human_size(st->st_size, sbuf, sizeof(sbuf));
        printf("%8s ", sbuf);
    } else {
        printf("%8lld ", (long long)st->st_size);
    }

    /* Modification time */
    char timebuf[32];
    struct tm *tm = localtime(&st->st_mtime);
    if (tm) strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm);
    else    snprintf(timebuf, sizeof(timebuf), "?");

    printf("%s %s", timebuf, name);

    /* Symlink target */
    if (S_ISLNK(st->st_mode)) {
        char target[512];
        ssize_t n = readlink(path, target, sizeof(target) - 1);
        if (n >= 0) {
            target[n] = '\0';
            printf(" -> %s", target);
        }
    }

    printf("\n");
    (void)path;
}

static void list_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "ls: cannot open '%s': %s\n", path, strerror(errno));
        return;
    }

    entry_t *entries = NULL;
    int count = 0, capacity = 64;
    entries = malloc((size_t)capacity * sizeof(entry_t));
    if (!entries) {
        closedir(dir);
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!opt_all && ent->d_name[0] == '.') continue;

        if (count >= capacity) {
            capacity *= 2;
            entry_t *tmp = realloc(entries, (size_t)capacity * sizeof(entry_t));
            if (!tmp) break;
            entries = tmp;
        }

        strncpy(entries[count].name, ent->d_name, sizeof(entries[count].name) - 1);
        entries[count].name[sizeof(entries[count].name) - 1] = '\0';

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ent->d_name);
        if (lstat(fullpath, &entries[count].st) < 0) {
            memset(&entries[count].st, 0, sizeof(entries[count].st));
        }
        count++;
    }
    closedir(dir);

    /* Sort */
    if (opt_time_sort) {
        qsort(entries, (size_t)count, sizeof(entry_t), cmp_mtime);
    } else {
        qsort(entries, (size_t)count, sizeof(entry_t), cmp_name);
    }

    for (int i = 0; i < count; i++) {
        int idx = opt_reverse ? (count - 1 - i) : i;
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entries[idx].name);
        print_entry(fullpath, entries[idx].name, &entries[idx].st);
    }

    free(entries);
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *paths[256];
    int path_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("ls version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0' && argv[i][1] != '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'l': opt_long = 1; break;
                    case 'a': opt_all = 1; break;
                    case '1': break; /* one per line is default */
                    case 'h': opt_human = 1; break;
                    case 'r': opt_reverse = 1; break;
                    case 't': opt_time_sort = 1; break;
                    case 'v': verbose = 1; break;
                    default:
                        fprintf(stderr, "ls: invalid option -- '%c'\n", argv[i][j]);
                        return 1;
                }
            }
        } else if (argv[i][0] != '-') {
            if (path_count < 256) paths[path_count++] = argv[i];
        } else {
            fprintf(stderr, "ls: unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    if (path_count == 0) {
        paths[path_count++] = ".";
    }

    for (int i = 0; i < path_count; i++) {
        struct stat st;
        if (lstat(paths[i], &st) < 0) {
            fprintf(stderr, "ls: cannot access '%s': %s\n", paths[i], strerror(errno));
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            if (path_count > 1) printf("%s:\n", paths[i]);
            list_dir(paths[i]);
            if (path_count > 1 && i < path_count - 1) printf("\n");
        } else {
            const char *base = strrchr(paths[i], '/');
            base = base ? base + 1 : paths[i];
            print_entry(paths[i], base, &st);
        }
    }

    return 0;
}
