/*
 * chmod - change file mode bits
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
static int opt_recursive = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] MODE FILE...\n", progname);
    fprintf(stderr, "Change file mode bits.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -R, --recursive  Change files and directories recursively\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
    fprintf(stderr, "\nMODE is octal (e.g., 0755) or symbolic (e.g., u+x,g-w).\n");
}

static mode_t parse_octal_mode(const char *str) {
    return (mode_t)strtol(str, NULL, 8);
}

static int chmod_file(const char *path, mode_t mode);

static int chmod_recursive(const char *path, mode_t mode) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        fprintf(stderr, "chmod: cannot access '%s': %s\n", path, strerror(errno));
        return -1;
    }

    if (chmod(path, mode) < 0) {
        fprintf(stderr, "chmod: changing permissions of '%s': %s\n", path, strerror(errno));
        return -1;
    }

    if (verbose >= 1) {
        printf("mode of '%s' changed to %04o\n", path, mode);
    }

    if (S_ISDIR(st.st_mode) && opt_recursive) {
        /* Recursively process directory contents */
        char cmd[2048];
        snprintf(cmd, sizeof(cmd), "find '%s' -mindepth 1", path);
        FILE *fp = popen(cmd, "r");
        if (!fp) return -1;

        char entry[1024];
        while (fgets(entry, sizeof(entry), fp)) {
            /* Remove newline */
            size_t len = strlen(entry);
            if (len > 0 && entry[len - 1] == '\n') {
                entry[len - 1] = '\0';
            }
            chmod_file(entry, mode);
        }
        pclose(fp);
    }

    return 0;
}

static int chmod_file(const char *path, mode_t mode) {
    return chmod_recursive(path, mode);
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *mode_str = NULL;
    char *files[256];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("chmod version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "--recursive") == 0) {
            opt_recursive = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "chmod: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (!mode_str) {
                mode_str = argv[i];
            } else {
                if (file_count < 256) files[file_count++] = argv[i];
            }
        }
    }

    if (!mode_str || file_count == 0) {
        fprintf(stderr, "chmod: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    mode_t mode = parse_octal_mode(mode_str);

    int ret = 0;
    for (int i = 0; i < file_count; i++) {
        if (chmod_file(files[i], mode) < 0) {
            ret = 1;
        }
    }

    return ret;
}
