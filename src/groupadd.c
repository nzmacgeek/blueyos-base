/*
 * groupadd - create a new group
 * Part of BlueyOS base utilities
 *
 * A simplified implementation for creating groups
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#define MAX_LINE 1024
#define GROUP_FILE "/etc/group"
#define MAX_GID 59999

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] GROUP\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -g, --gid GID         Use GID for the new group\n");
    fprintf(stderr, "  -r, --system          Create a system group\n");
    fprintf(stderr, "  -v, --verbose         Verbose output\n");
    fprintf(stderr, "  --version             Print version\n");
    fprintf(stderr, "  -h, --help            Show this help\n");
}

/* Check if group name already exists */
static int group_exists(const char *groupname) {
    struct group *grp = getgrnam(groupname);
    return (grp != NULL);
}

/* Check if GID already exists */
static int gid_exists(gid_t gid) {
    struct group *grp = getgrgid(gid);
    return (grp != NULL);
}

/* Find next available GID */
static gid_t find_next_gid(int system_group) {
    FILE *fp;
    char line[MAX_LINE];
    gid_t max_gid = system_group ? 99 : 999;
    gid_t gid_limit = system_group ? 999 : 60000;

    fp = fopen(GROUP_FILE, "r");
    if (!fp) {
        return max_gid;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *name, *pass, *gid_str;
        gid_t gid;

        name = strtok(line, ":");
        pass = strtok(NULL, ":");
        gid_str = strtok(NULL, ":");

        if (!name || !pass || !gid_str) {
            continue;
        }

        gid = (gid_t)atoi(gid_str);

        if (system_group) {
            if (gid >= 100 && gid < 1000 && gid > max_gid) {
                max_gid = gid;
            }
        } else {
            if (gid >= 1000 && gid < 60000 && gid > max_gid) {
                max_gid = gid;
            }
        }
    }

    fclose(fp);

    /* Return next available GID */
    max_gid++;
    if (max_gid >= gid_limit) {
        return 0; /* No available GID */
    }

    return max_gid;
}

/* Add group to /etc/group atomically via temp file */
static int add_group(const char *groupname, gid_t gid) {
    char tmppath[] = "/etc/.group.XXXXXX";
    int tmpfd = mkstemp(tmppath);
    if (tmpfd < 0) {
        fprintf(stderr, "groupadd: cannot create temp file: %s\n", strerror(errno));
        return -1;
    }

    /* Copy existing content */
    FILE *in = fopen(GROUP_FILE, "r");
    FILE *out = fdopen(tmpfd, "w");
    if (!out) {
        fprintf(stderr, "groupadd: cannot open temp file: %s\n", strerror(errno));
        close(tmpfd);
        unlink(tmppath);
        return -1;
    }

    if (in) {
        char buf[MAX_LINE];
        while (fgets(buf, sizeof(buf), in)) {
            fputs(buf, out);
        }
        fclose(in);
    }

    /* Append new group */
    fprintf(out, "%s:x:%u:\n", groupname, (unsigned int)gid);
    fclose(out);

    if (rename(tmppath, GROUP_FILE) < 0) {
        fprintf(stderr, "groupadd: cannot replace %s: %s\n", GROUP_FILE, strerror(errno));
        unlink(tmppath);
        return -1;
    }

    if (verbose >= 2) {
        fprintf(stderr, "Group '%s' added with GID %u\n", groupname, (unsigned int)gid);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int i;
    char *groupname = NULL;
    gid_t gid = 0;
    int gid_specified = 0;
    int system_group = 0;

    /* Check for VERBOSE environment variable */
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Parse arguments - check for version/help first before requiring root */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("groupadd version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    /* Check if running as root */
    if (getuid() != 0 && geteuid() != 0) {
        fprintf(stderr, "groupadd: permission denied (must be root)\n");
        return 1;
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--system") == 0) {
            system_group = 1;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gid") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "groupadd: -g requires an argument\n");
                return 1;
            }
            char *endptr;
            unsigned long val = strtoul(argv[i], &endptr, 10);
            if (*endptr != '\0' || val > MAX_GID) {
                fprintf(stderr, "groupadd: invalid GID '%s'\n", argv[i]);
                return 1;
            }
            gid = (gid_t)val;
            gid_specified = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "groupadd: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            /* It's the group name */
            groupname = argv[i];
        }
    }

    if (!groupname) {
        fprintf(stderr, "groupadd: no group name specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Check if group already exists */
    if (group_exists(groupname)) {
        fprintf(stderr, "groupadd: group '%s' already exists\n", groupname);
        return 9;
    }

    /* If GID not specified, find next available */
    if (!gid_specified) {
        gid = find_next_gid(system_group);
        if (gid == 0) {
            fprintf(stderr, "groupadd: no available GID found\n");
            return 4;
        }
    } else {
        /* Check if specified GID already exists */
        if (gid_exists(gid)) {
            fprintf(stderr, "groupadd: GID %u already exists\n", (unsigned int)gid);
            return 4;
        }
    }

    if (verbose >= 1) {
        fprintf(stderr, "Creating group '%s' with GID %u\n", groupname, (unsigned int)gid);
    }

    /* Add the group */
    if (add_group(groupname, gid) < 0) {
        return 10;
    }

    if (verbose >= 1) {
        printf("Group '%s' created successfully\n", groupname);
    }

    return 0;
}
