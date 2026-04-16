/*
 * mv - move or rename files
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_force = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] SOURCE DEST\n", progname);
    fprintf(stderr, "   or: %s [OPTIONS] SOURCE... DIRECTORY\n", progname);
    fprintf(stderr, "Rename SOURCE to DEST, or move SOURCE(s) to DIRECTORY.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -f, --force      Force overwrite\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

/* Forward declarations */
static int copy_path(const char *src, const char *dst);
static int remove_path(const char *path);

static int copy_file_data(const char *src, const char *dst) {
    int sfd = open(src, O_RDONLY);
    if (sfd < 0) {
        fprintf(stderr, "mv: cannot open '%s': %s\n", src, strerror(errno));
        return -1;
    }

    struct stat st;
    if (fstat(sfd, &st) < 0) {
        fprintf(stderr, "mv: cannot stat '%s': %s\n", src, strerror(errno));
        close(sfd);
        return -1;
    }

    int dfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (dfd < 0) {
        fprintf(stderr, "mv: cannot create '%s': %s\n", dst, strerror(errno));
        close(sfd);
        return -1;
    }

    char buf[8192];
    ssize_t n;
    while ((n = read(sfd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t rc = write(dfd, buf + written, (size_t)(n - written));
            if (rc < 0) {
                if (errno == EINTR) continue;
                fprintf(stderr, "mv: write error '%s': %s\n", dst, strerror(errno));
                close(sfd);
                close(dfd);
                return -1;
            }
            written += rc;
        }
    }

    if (n < 0) {
        fprintf(stderr, "mv: read error '%s': %s\n", src, strerror(errno));
        close(sfd);
        close(dfd);
        return -1;
    }

    close(sfd);
    close(dfd);
    return 0;
}

static int copy_path(const char *src, const char *dst) {
    struct stat st;
    if (lstat(src, &st) < 0) {
        fprintf(stderr, "mv: cannot stat '%s': %s\n", src, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        if (mkdir(dst, st.st_mode & 0777) < 0 && errno != EEXIST) {
            fprintf(stderr, "mv: cannot create directory '%s': %s\n", dst, strerror(errno));
            return -1;
        }

        DIR *dir = opendir(src);
        if (!dir) {
            fprintf(stderr, "mv: cannot open directory '%s': %s\n", src, strerror(errno));
            return -1;
        }

        int ret = 0;
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            char src_path[1024], dst_path[1024];
            snprintf(src_path, sizeof(src_path), "%s/%s", src, ent->d_name);
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, ent->d_name);
            if (copy_path(src_path, dst_path) < 0) ret = -1;
        }
        closedir(dir);
        return ret;
    } else if (S_ISLNK(st.st_mode)) {
        char target[1024];
        ssize_t len = readlink(src, target, sizeof(target) - 1);
        if (len < 0) {
            fprintf(stderr, "mv: cannot read link '%s': %s\n", src, strerror(errno));
            return -1;
        }
        target[len] = '\0';
        if (symlink(target, dst) < 0) {
            fprintf(stderr, "mv: cannot create symlink '%s': %s\n", dst, strerror(errno));
            return -1;
        }
        return 0;
    } else {
        return copy_file_data(src, dst);
    }
}

static int remove_path(const char *path) {
    struct stat st;
    if (lstat(path, &st) < 0) return -1;

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) return -1;

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);
            remove_path(full_path);
        }
        closedir(dir);
        return rmdir(path);
    } else {
        return unlink(path);
    }
}

static int move_file(const char *src, const char *dst) {
    /* Try rename first (same filesystem) */
    if (rename(src, dst) == 0) {
        if (verbose >= 1) {
            printf("'%s' -> '%s'\n", src, dst);
        }
        return 0;
    }

    /* If EXDEV (cross-device), need to copy and delete */
    if (errno != EXDEV) {
        if (errno == EEXIST && opt_force) {
            unlink(dst);
            if (rename(src, dst) == 0) {
                if (verbose >= 1) {
                    printf("'%s' -> '%s'\n", src, dst);
                }
                return 0;
            }
        }
        fprintf(stderr, "mv: cannot move '%s' to '%s': %s\n", src, dst, strerror(errno));
        return -1;
    }

    /* Cross-device move: copy then delete in-process */
    if (copy_path(src, dst) < 0) {
        fprintf(stderr, "mv: failed to copy '%s' to '%s'\n", src, dst);
        return -1;
    }

    if (remove_path(src) < 0) {
        fprintf(stderr, "mv: warning: failed to remove '%s': %s\n", src, strerror(errno));
    }

    if (verbose >= 1) {
        printf("'%s' -> '%s'\n", src, dst);
    }

    return 0;
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
            printf("mv version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            opt_force = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "mv: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (source_count < 256) sources[source_count++] = argv[i];
        }
    }

    if (source_count < 2) {
        fprintf(stderr, "mv: missing file operand\n");
        print_usage(argv[0]);
        return 1;
    }

    const char *dest = sources[source_count - 1];
    source_count--;

    /* Check if dest is a directory */
    struct stat dest_st;
    int dest_is_dir = (stat(dest, &dest_st) == 0 && S_ISDIR(dest_st.st_mode));

    if (source_count > 1 && !dest_is_dir) {
        fprintf(stderr, "mv: target '%s' is not a directory\n", dest);
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

        if (move_file(sources[i], dst_path) < 0) {
            ret = 1;
        }
    }

    return ret;
}
