/*
 * chown - change file owner and group
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;
static int opt_recursive = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [OWNER][:[GROUP]] FILE...\n", progname);
    fprintf(stderr, "Change the owner and/or group of each FILE.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -R, --recursive  Change files and directories recursively\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static uid_t parse_owner(const char *name) {
    if (!name || !*name) return (uid_t)-1;

    /* Try numeric ID first */
    char *endptr;
    long id = strtol(name, &endptr, 10);
    if (*endptr == '\0') {
        return (uid_t)id;
    }

    /* Lookup user by name */
    struct passwd *pw = getpwnam(name);
    if (pw) {
        return pw->pw_uid;
    }

    fprintf(stderr, "chown: invalid user '%s'\n", name);
    return (uid_t)-1;
}

static gid_t parse_group(const char *name) {
    if (!name || !*name) return (gid_t)-1;

    /* Try numeric ID first */
    char *endptr;
    long id = strtol(name, &endptr, 10);
    if (*endptr == '\0') {
        return (gid_t)id;
    }

    /* Lookup group by name */
    struct group *gr = getgrnam(name);
    if (gr) {
        return gr->gr_gid;
    }

    fprintf(stderr, "chown: invalid group '%s'\n", name);
    return (gid_t)-1;
}

static int chown_file(const char *path, uid_t uid, gid_t gid);

static int chown_recursive(const char *path, uid_t uid, gid_t gid) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        fprintf(stderr, "chown: cannot access '%s': %s\n", path, strerror(errno));
        return -1;
    }

    if (lchown(path, uid, gid) < 0) {
        fprintf(stderr, "chown: changing ownership of '%s': %s\n", path, strerror(errno));
        return -1;
    }

    if (verbose >= 1) {
        printf("ownership of '%s' changed", path);
        if (uid != (uid_t)-1) printf(" to %u", uid);
        if (gid != (gid_t)-1) printf(":%u", gid);
        printf("\n");
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
            chown_file(entry, uid, gid);
        }
        pclose(fp);
    }

    return 0;
}

static int chown_file(const char *path, uid_t uid, gid_t gid) {
    return chown_recursive(path, uid, gid);
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *owner_spec = NULL;
    char *files[256];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("chown version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "--recursive") == 0) {
            opt_recursive = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "chown: invalid option '%s'\n", argv[i]);
            return 1;
        } else {
            if (!owner_spec) {
                owner_spec = argv[i];
            } else {
                if (file_count < 256) files[file_count++] = argv[i];
            }
        }
    }

    if (!owner_spec || file_count == 0) {
        fprintf(stderr, "chown: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Parse owner:group */
    uid_t uid = (uid_t)-1;
    gid_t gid = (gid_t)-1;

    char *colon = strchr(owner_spec, ':');
    if (colon) {
        *colon = '\0';
        if (*owner_spec) {
            uid = parse_owner(owner_spec);
            if (uid == (uid_t)-1 && *owner_spec) return 1;
        }
        if (*(colon + 1)) {
            gid = parse_group(colon + 1);
            if (gid == (gid_t)-1) return 1;
        }
    } else {
        uid = parse_owner(owner_spec);
        if (uid == (uid_t)-1) return 1;
    }

    int ret = 0;
    for (int i = 0; i < file_count; i++) {
        if (chown_file(files[i], uid, gid) < 0) {
            ret = 1;
        }
    }

    return ret;
}
