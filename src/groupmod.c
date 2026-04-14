/*
 * groupmod - modify a group
 * Part of BlueyOS base utilities
 *
 * A simplified implementation for modifying groups
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

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] GROUP\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -g, --gid GID         Change the group ID to GID\n");
    fprintf(stderr, "  -n, --new-name NAME   Change the group name to NAME\n");
    fprintf(stderr, "  -v, --verbose         Verbose output\n");
    fprintf(stderr, "  --version             Print version\n");
    fprintf(stderr, "  -h, --help            Show this help\n");
}

/* Check if group name already exists */
static int group_exists(const char *groupname) {
    struct group *grp = getgrnam(groupname);
    return (grp != NULL);
}

/* Modify group in /etc/group */
static int modify_group(const char *oldname, const char *newname, gid_t new_gid, int change_gid) {
    FILE *in, *out;
    char line[MAX_LINE];
    int found = 0;

    in = fopen(GROUP_FILE, "r");
    if (!in) {
        fprintf(stderr, "groupmod: cannot open %s: %s\n", GROUP_FILE, strerror(errno));
        return -1;
    }

    char tmppath[] = "/etc/.group.XXXXXX";
    int tmpfd = mkstemp(tmppath);
    if (tmpfd < 0) {
        fprintf(stderr, "groupmod: cannot create temp file: %s\n", strerror(errno));
        fclose(in);
        return -1;
    }
    out = fdopen(tmpfd, "w");
    if (!out) {
        fprintf(stderr, "groupmod: cannot open temp file: %s\n", strerror(errno));
        close(tmpfd);
        unlink(tmppath);
        fclose(in);
        return -1;
    }

    while (fgets(line, sizeof(line), in)) {
        char *name, *pass, *gid_str, *members;
        char line_copy[MAX_LINE];

        /* Make a copy for parsing */
        strncpy(line_copy, line, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';

        /* Remove trailing newline from copy */
        size_t len = strlen(line_copy);
        if (len > 0 && line_copy[len - 1] == '\n') {
            line_copy[len - 1] = '\0';
        }

        /* Parse group line */
        name = strtok(line_copy, ":");
        pass = strtok(NULL, ":");
        gid_str = strtok(NULL, ":");
        members = strtok(NULL, ":");

        if (!name || !pass || !gid_str) {
            fprintf(stderr, "groupmod: malformed line in %s\n", GROUP_FILE);
            fclose(in);
            fclose(out);
            unlink(tmppath);
            return -1;
        }

        if (strcmp(name, oldname) == 0) {
            /* This is the group to modify */
            found = 1;

            const char *final_name = newname ? newname : name;
            unsigned int final_gid = change_gid ? (unsigned int)new_gid : (unsigned int)atoi(gid_str);

            if (verbose >= 2) {
                fprintf(stderr, "Modifying group '%s'\n", oldname);
            }

            /* Write modified line */
            if (members && strlen(members) > 0) {
                fprintf(out, "%s:%s:%u:%s\n", final_name, pass, final_gid, members);
            } else {
                fprintf(out, "%s:%s:%u:\n", final_name, pass, final_gid);
            }
        } else {
            /* Write original line */
            fputs(line, out);
        }
    }

    fclose(in);
    fclose(out);

    if (!found) {
        fprintf(stderr, "groupmod: group '%s' does not exist\n", oldname);
        unlink(tmppath);
        return -1;
    }

    /* Replace original with temp file */
    if (rename(tmppath, GROUP_FILE) < 0) {
        fprintf(stderr, "groupmod: cannot replace %s: %s\n", GROUP_FILE, strerror(errno));
        unlink(tmppath);
        return -1;
    }

    if (verbose >= 2) {
        fprintf(stderr, "Group file updated successfully\n");
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int i;
    char *groupname = NULL;
    char *new_name = NULL;
    gid_t new_gid = 0;
    int change_gid = 0;

    /* Check for VERBOSE environment variable */
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Parse arguments - check for version/help first before requiring root */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("groupmod version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    /* Check if running as root */
    if (getuid() != 0 && geteuid() != 0) {
        fprintf(stderr, "groupmod: permission denied (must be root)\n");
        return 1;
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gid") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "groupmod: -g requires an argument\n");
                return 1;
            }
            char *endptr;
            unsigned long val = strtoul(argv[i], &endptr, 10);
            if (*endptr != '\0' || val > 59999) {
                fprintf(stderr, "groupmod: invalid GID '%s'\n", argv[i]);
                return 1;
            }
            new_gid = (gid_t)val;
            change_gid = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--new-name") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "groupmod: -n requires an argument\n");
                return 1;
            }
            new_name = argv[i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "groupmod: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            /* It's the group name */
            groupname = argv[i];
        }
    }

    if (!groupname) {
        fprintf(stderr, "groupmod: no group name specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Check if original group exists */
    if (!group_exists(groupname)) {
        fprintf(stderr, "groupmod: group '%s' does not exist\n", groupname);
        return 6;
    }

    /* If changing name, check if new name already exists */
    if (new_name && strcmp(new_name, groupname) != 0) {
        if (group_exists(new_name)) {
            fprintf(stderr, "groupmod: group '%s' already exists\n", new_name);
            return 9;
        }
    }

    /* If changing GID, check if new GID already exists */
    if (change_gid) {
        struct group *existing_grp = getgrgid(new_gid);
        if (existing_grp != NULL && strcmp(existing_grp->gr_name, groupname) != 0) {
            fprintf(stderr, "groupmod: GID %u already exists\n", (unsigned int)new_gid);
            return 4;
        }
    }

    if (!change_gid && !new_name) {
        fprintf(stderr, "groupmod: no changes specified\n");
        return 1;
    }

    if (verbose >= 1) {
        if (new_name && change_gid) {
            fprintf(stderr, "Modifying group '%s' to '%s' with GID %u\n",
                    groupname, new_name, (unsigned int)new_gid);
        } else if (new_name) {
            fprintf(stderr, "Renaming group '%s' to '%s'\n", groupname, new_name);
        } else if (change_gid) {
            fprintf(stderr, "Changing GID of group '%s' to %u\n",
                    groupname, (unsigned int)new_gid);
        }
    }

    /* Modify the group */
    if (modify_group(groupname, new_name, new_gid, change_gid) < 0) {
        return 10;
    }

    if (verbose >= 1) {
        printf("Group modified successfully\n");
    }

    return 0;
}
