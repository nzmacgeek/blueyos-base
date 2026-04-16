/*
 * cp - copy files and directories
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <time.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_recursive = 0;
static int opt_force = 0;
static int opt_preserve = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] SOURCE... DEST\n", progname);
    fprintf(stderr, "Copy SOURCE to DEST, or multiple SOURCE(s) to DIRECTORY.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -r, -R, --recursive  Copy directories recursively\n");
    fprintf(stderr, "  -f, --force          Force overwrite\n");
    fprintf(stderr, "  -p, --preserve       Preserve mode, ownership, timestamps\n");
    fprintf(stderr, "  -v, --verbose        Verbose output\n");
    fprintf(stderr, "  --version            Print version\n");
    fprintf(stderr, "  -h, --help           Show this help\n");
}

static int copy_file(const char *src, const char *dst) {
    int sfd = open(src, O_RDONLY);
    if (sfd < 0) {
        fprintf(stderr, "cp: cannot open '%s': %s\n", src, strerror(errno));
        return -1;
    }

    struct stat st;
    if (fstat(sfd, &st) < 0) {
        fprintf(stderr, "cp: cannot stat '%s': %s\n", src, strerror(errno));
        close(sfd);
        return -1;
    }

    int dfd = open(dst, O_WRONLY | O_CREAT | (opt_force ? O_TRUNC : O_EXCL), 0644);
    if (dfd < 0) {
        if (errno == EEXIST && opt_force) {
            unlink(dst);
            dfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        if (dfd < 0) {
            fprintf(stderr, "cp: cannot create '%s': %s\n", dst, strerror(errno));
            close(sfd);
            return -1;
        }
    }

    char buf[8192];
    ssize_t n;
    while ((n = read(sfd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;

        while (written < n) {
            ssize_t rc = write(dfd, buf + written, (size_t)(n - written));
            if (rc < 0) {
                if (errno == EINTR) {
                    continue;
                }
                fprintf(stderr, "cp: write error '%s': %s\n", dst, strerror(errno));
                close(sfd);
                close(dfd);
                return -1;
            }
            written += rc;
        }
    }

    if (n < 0) {
        fprintf(stderr, "cp: read error '%s': %s\n", src, strerror(errno));
        close(sfd);
        close(dfd);
        return -1;
    }

    if (opt_preserve) {
        fchmod(dfd, st.st_mode);
        fchown(dfd, st.st_uid, st.st_gid);
        struct timespec times[2];
        times[0] = st.st_atim;
        times[1] = st.st_mtim;
        futimens(dfd, times);
    }

    close(sfd);
    close(dfd);

    if (verbose >= 1) {
        printf("'%s' -> '%s'\n", src, dst);
    }

    return 0;
}

static int copy_recursive(const char *src, const char *dst);

static int copy_dir(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) < 0) {
        fprintf(stderr, "cp: cannot stat '%s': %s\n", src, strerror(errno));
        return -1;
    }

    if (mkdir(dst, 0755) < 0 && errno != EEXIST) {
        fprintf(stderr, "cp: cannot create directory '%s': %s\n", dst, strerror(errno));
        return -1;
    }

    DIR *dir = opendir(src);
    if (!dir) {
        fprintf(stderr, "cp: cannot open directory '%s': %s\n", src, strerror(errno));
        return -1;
    }

    int ret = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char src_path[1024], dst_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, ent->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, ent->d_name);

        if (copy_recursive(src_path, dst_path) < 0) {
            ret = -1;
        }
    }

    closedir(dir);

    if (opt_preserve) {
        chmod(dst, st.st_mode);
        chown(dst, st.st_uid, st.st_gid);
        struct timespec times[2];
        times[0] = st.st_atim;
        times[1] = st.st_mtim;
        utimensat(AT_FDCWD, dst, times, 0);
    }

    return ret;
}

static int copy_recursive(const char *src, const char *dst) {
    struct stat st;
    if (lstat(src, &st) < 0) {
        fprintf(stderr, "cp: cannot stat '%s': %s\n", src, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        if (!opt_recursive) {
            fprintf(stderr, "cp: omitting directory '%s' (use -r)\n", src);
            return -1;
        }
        return copy_dir(src, dst);
    } else if (S_ISLNK(st.st_mode)) {
        char target[1024];
        ssize_t len = readlink(src, target, sizeof(target) - 1);
        if (len < 0) {
            fprintf(stderr, "cp: cannot read link '%s': %s\n", src, strerror(errno));
            return -1;
        }
        target[len] = '\0';

        if (symlink(target, dst) < 0) {
            if (errno == EEXIST && opt_force) {
                if (unlink(dst) < 0) {
                    fprintf(stderr, "cp: cannot remove '%s': %s\n", dst, strerror(errno));
                    return -1;
                }
                if (symlink(target, dst) < 0) {
                    fprintf(stderr, "cp: cannot create symlink '%s': %s\n", dst, strerror(errno));
                    return -1;
                }
            } else {
                fprintf(stderr, "cp: cannot create symlink '%s': %s\n", dst, strerror(errno));
                return -1;
            }
        }

        if (verbose >= 1) {
            printf("'%s' -> '%s'\n", src, dst);
        }
        return 0;
    } else {
        return copy_file(src, dst);
    }
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *sources[256];
    int source_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("cp version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "-R") == 0 ||
                   strcmp(argv[i], "--recursive") == 0) {
            opt_recursive = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            opt_force = 1;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--preserve") == 0) {
            opt_preserve = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "cp: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (source_count < 256) sources[source_count++] = argv[i];
        }
    }

    if (source_count < 2) {
        fprintf(stderr, "cp: missing file operand\n");
        print_usage(argv[0]);
        return 1;
    }

    const char *dest = sources[source_count - 1];
    source_count--;

    /* Check if dest is a directory */
    struct stat dest_st;
    int dest_is_dir = (stat(dest, &dest_st) == 0 && S_ISDIR(dest_st.st_mode));

    if (source_count > 1 && !dest_is_dir) {
        fprintf(stderr, "cp: target '%s' is not a directory\n", dest);
        return 1;
    }

    int ret = 0;
    for (int i = 0; i < source_count; i++) {
        char dst_path[1024];
        if (dest_is_dir) {
            const char *base = strrchr(sources[i], '/');
            base = base ? base + 1 : sources[i];
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dest, base);
        } else {
            strncpy(dst_path, dest, sizeof(dst_path) - 1);
            dst_path[sizeof(dst_path) - 1] = '\0';
        }

        if (copy_recursive(sources[i], dst_path) < 0) {
            ret = 1;
        }
    }

    return ret;
}
