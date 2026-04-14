/*
 * groupdel - delete a group
 * Part of BlueyOS base utilities
 *
 * A simplified implementation for deleting groups
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#define MAX_LINE 1024
#define GROUP_FILE "/etc/group"

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] GROUP\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose         Verbose output\n");
    fprintf(stderr, "  --version             Print version\n");
    fprintf(stderr, "  -h, --help            Show this help\n");
}

/* Check if any user has this group as their primary group */
static int group_is_primary(const char *groupname) {
    struct group *grp;
    struct passwd *pwd;
    gid_t gid;

    grp = getgrnam(groupname);
    if (!grp) {
        return 0;
    }

    gid = grp->gr_gid;

    /* Check all users */
    setpwent();
    while ((pwd = getpwent()) != NULL) {
        if (pwd->pw_gid == gid) {
            endpwent();
            return 1;
        }
    }
    endpwent();

    return 0;
}

/* Delete group from /etc/group */
static int delete_group(const char *groupname) {
    FILE *in, *out;
    char line[MAX_LINE];
    int found = 0;

    in = fopen(GROUP_FILE, "r");
    if (!in) {
        fprintf(stderr, "groupdel: cannot open %s: %s\n", GROUP_FILE, strerror(errno));
        return -1;
    }

    char tmppath[] = "/etc/.group.XXXXXX";
    int tmpfd = mkstemp(tmppath);
    if (tmpfd < 0) {
        fprintf(stderr, "groupdel: cannot create temp file: %s\n", strerror(errno));
        fclose(in);
        return -1;
    }
    out = fdopen(tmpfd, "w");
    if (!out) {
        fprintf(stderr, "groupdel: cannot open temp file: %s\n", strerror(errno));
        close(tmpfd);
        unlink(tmppath);
        fclose(in);
        return -1;
    }

    while (fgets(line, sizeof(line), in)) {
        char line_copy[MAX_LINE];
        char *name;

        /* Make a copy for parsing */
        strncpy(line_copy, line, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';

        /* Parse group name */
        name = strtok(line_copy, ":");

        if (name && strcmp(name, groupname) == 0) {
            /* Skip this line (delete the group) */
            found = 1;
            if (verbose >= 2) {
                fprintf(stderr, "Deleting group '%s'\n", groupname);
            }
            continue;
        }

        /* Write all other lines */
        fputs(line, out);
    }

    fclose(in);
    fclose(out);

    if (!found) {
        fprintf(stderr, "groupdel: group '%s' does not exist\n", groupname);
        unlink(tmppath);
        return -1;
    }

    /* Replace original with temp file */
    if (rename(tmppath, GROUP_FILE) < 0) {
        fprintf(stderr, "groupdel: cannot replace %s: %s\n", GROUP_FILE, strerror(errno));
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

    /* Check for VERBOSE environment variable */
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Parse arguments - check for version/help first before requiring root */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("groupdel version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    /* Check if running as root */
    if (getuid() != 0 && geteuid() != 0) {
        fprintf(stderr, "groupdel: permission denied (must be root)\n");
        return 1;
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "groupdel: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            /* It's the group name */
            groupname = argv[i];
        }
    }

    if (!groupname) {
        fprintf(stderr, "groupdel: no group name specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Check if group is a primary group for any user */
    if (group_is_primary(groupname)) {
        fprintf(stderr, "groupdel: cannot remove group '%s', it is a user's primary group\n", groupname);
        return 8;
    }

    if (verbose >= 1) {
        fprintf(stderr, "Deleting group '%s'\n", groupname);
    }

    /* Delete the group */
    if (delete_group(groupname) < 0) {
        return 6;
    }

    if (verbose >= 1) {
        printf("Group '%s' deleted successfully\n", groupname);
    }

    return 0;
}
