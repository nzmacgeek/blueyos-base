/*
 * mv - move or rename files
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
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

    /* Cross-device move: copy then delete */
    /* For simplicity, we'll just use system cp and rm */
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "cp -r %s '%s' '%s'", opt_force ? "-f" : "", src, dst);
    if (system(cmd) != 0) {
        fprintf(stderr, "mv: failed to copy '%s' to '%s'\n", src, dst);
        return -1;
    }

    struct stat st;
    if (stat(src, &st) == 0 && S_ISDIR(st.st_mode)) {
        snprintf(cmd, sizeof(cmd), "rm -rf '%s'", src);
    } else {
        snprintf(cmd, sizeof(cmd), "rm -f '%s'", src);
    }

    if (system(cmd) != 0) {
        fprintf(stderr, "mv: warning: failed to remove '%s'\n", src);
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
