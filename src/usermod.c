/*
 * usermod - modify user account and group membership
 * Part of BlueyOS base utilities
 *
 * A simplified implementation for managing group membership
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

#define MAX_LINE 1024
#define GROUP_FILE "/etc/group"
#define GROUP_TEMP "/etc/group.tmp"

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] USERNAME\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a, --append          Add user to supplementary groups (use with -G)\n");
    fprintf(stderr, "  -G, --groups GROUPS   Set supplementary groups (comma-separated)\n");
    fprintf(stderr, "  -aG GROUP             Add user to a group\n");
    fprintf(stderr, "  -rG GROUP             Remove user from a group\n");
    fprintf(stderr, "  -v, --verbose         Verbose output\n");
    fprintf(stderr, "  --version             Print version\n");
    fprintf(stderr, "  -h, --help            Show this help\n");
}

/* Check if a user exists in the group member list */
static int user_in_list(const char *user, const char *members) {
    char *list_copy, *token, *saveptr;
    int found = 0;

    if (!members || strlen(members) == 0) {
        return 0;
    }

    list_copy = strdup(members);
    if (!list_copy) {
        return 0;
    }

    token = strtok_r(list_copy, ",", &saveptr);
    while (token) {
        if (strcmp(token, user) == 0) {
            found = 1;
            break;
        }
        token = strtok_r(NULL, ",", &saveptr);
    }

    free(list_copy);
    return found;
}

/* Add a user to a group member list */
static char *add_user_to_list(const char *user, const char *members) {
    char *new_list;
    size_t len;

    /* Check if user already in list */
    if (user_in_list(user, members)) {
        return strdup(members);
    }

    /* Calculate new list length */
    if (members && strlen(members) > 0) {
        len = strlen(members) + strlen(user) + 2; /* +2 for comma and null */
        new_list = malloc(len);
        if (!new_list) {
            return NULL;
        }
        snprintf(new_list, len, "%s,%s", members, user);
    } else {
        new_list = strdup(user);
    }

    return new_list;
}

/* Remove a user from a group member list */
static char *remove_user_from_list(const char *user, const char *members) {
    char *list_copy, *token, *saveptr;
    char new_list[MAX_LINE];
    int first = 1;

    if (!members || strlen(members) == 0) {
        return strdup("");
    }

    list_copy = strdup(members);
    if (!list_copy) {
        return NULL;
    }

    new_list[0] = '\0';

    token = strtok_r(list_copy, ",", &saveptr);
    while (token) {
        if (strcmp(token, user) != 0) {
            if (!first) {
                strcat(new_list, ",");
            }
            strcat(new_list, token);
            first = 0;
        }
        token = strtok_r(NULL, ",", &saveptr);
    }

    free(list_copy);
    return strdup(new_list);
}

/* Modify group file to add or remove user from a group */
static int modify_group(const char *group_name, const char *username, int add) {
    FILE *in, *out;
    char line[MAX_LINE];
    char *name, *pass, *gid, *members;
    char *new_members;
    int found = 0;
    int modified = 0;

    in = fopen(GROUP_FILE, "r");
    if (!in) {
        fprintf(stderr, "usermod: cannot open %s: %s\n", GROUP_FILE, strerror(errno));
        return -1;
    }

    out = fopen(GROUP_TEMP, "w");
    if (!out) {
        fprintf(stderr, "usermod: cannot create %s: %s\n", GROUP_TEMP, strerror(errno));
        fclose(in);
        return -1;
    }

    while (fgets(line, sizeof(line), in)) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        /* Parse group line: name:password:gid:user_list */
        name = strtok(line, ":");
        pass = strtok(NULL, ":");
        gid = strtok(NULL, ":");
        members = strtok(NULL, ":");

        if (!name || !pass || !gid) {
            fprintf(stderr, "usermod: malformed line in %s\n", GROUP_FILE);
            fclose(in);
            fclose(out);
            unlink(GROUP_TEMP);
            return -1;
        }

        /* If this is the target group, modify it */
        if (strcmp(name, group_name) == 0) {
            found = 1;
            if (add) {
                new_members = add_user_to_list(username, members ? members : "");
                if (verbose >= 1) {
                    fprintf(stderr, "Adding user '%s' to group '%s'\n", username, group_name);
                }
            } else {
                new_members = remove_user_from_list(username, members ? members : "");
                if (verbose >= 1) {
                    fprintf(stderr, "Removing user '%s' from group '%s'\n", username, group_name);
                }
            }

            if (!new_members) {
                fprintf(stderr, "usermod: memory allocation failed\n");
                fclose(in);
                fclose(out);
                unlink(GROUP_TEMP);
                return -1;
            }

            /* Write modified line */
            if (strlen(new_members) > 0) {
                fprintf(out, "%s:%s:%s:%s\n", name, pass, gid, new_members);
            } else {
                fprintf(out, "%s:%s:%s:\n", name, pass, gid);
            }

            free(new_members);
            modified = 1;
        } else {
            /* Write original line */
            if (members) {
                fprintf(out, "%s:%s:%s:%s\n", name, pass, gid, members);
            } else {
                fprintf(out, "%s:%s:%s:\n", name, pass, gid);
            }
        }
    }

    fclose(in);
    fclose(out);

    if (!found) {
        fprintf(stderr, "usermod: group '%s' does not exist\n", group_name);
        unlink(GROUP_TEMP);
        return -1;
    }

    /* Replace original with temp file */
    if (rename(GROUP_TEMP, GROUP_FILE) < 0) {
        fprintf(stderr, "usermod: cannot replace %s: %s\n", GROUP_FILE, strerror(errno));
        unlink(GROUP_TEMP);
        return -1;
    }

    if (verbose >= 2) {
        fprintf(stderr, "Group file updated successfully\n");
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int i;
    char *username = NULL;
    char *groups = NULL;
    int append_mode = 0;
    int add_to_group = 0;
    int remove_from_group = 0;
    char *single_group = NULL;

    /* Check for VERBOSE environment variable */
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Parse arguments - check for version/help first before requiring root */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("usermod version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    /* Check if running as root */
    if (getuid() != 0 && geteuid() != 0) {
        fprintf(stderr, "usermod: permission denied (must be root)\n");
        return 1;
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--append") == 0) {
            append_mode = 1;
        } else if (strcmp(argv[i], "-G") == 0 || strcmp(argv[i], "--groups") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "usermod: -G requires an argument\n");
                return 1;
            }
            groups = argv[i];
        } else if (strcmp(argv[i], "-aG") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "usermod: -aG requires a group name\n");
                return 1;
            }
            add_to_group = 1;
            single_group = argv[i];
        } else if (strcmp(argv[i], "-rG") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "usermod: -rG requires a group name\n");
                return 1;
            }
            remove_from_group = 1;
            single_group = argv[i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "usermod: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            /* It's the username */
            username = argv[i];
        }
    }

    if (!username) {
        fprintf(stderr, "usermod: no username specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Verify user exists */
    struct passwd *pwd = getpwnam(username);
    if (!pwd) {
        fprintf(stderr, "usermod: user '%s' does not exist\n", username);
        return 6;
    }

    /* Handle -aG (add to single group) */
    if (add_to_group && single_group) {
        if (modify_group(single_group, username, 1) < 0) {
            return 6;
        }
        if (verbose >= 1) {
            printf("User '%s' added to group '%s'\n", username, single_group);
        }
        return 0;
    }

    /* Handle -rG (remove from single group) */
    if (remove_from_group && single_group) {
        if (modify_group(single_group, username, 0) < 0) {
            return 6;
        }
        if (verbose >= 1) {
            printf("User '%s' removed from group '%s'\n", username, single_group);
        }
        return 0;
    }

    /* Handle -G with comma-separated groups */
    if (groups) {
        char *group_copy = strdup(groups);
        char *token, *saveptr;

        if (!group_copy) {
            fprintf(stderr, "usermod: memory allocation failed\n");
            return 1;
        }

        token = strtok_r(group_copy, ",", &saveptr);
        while (token) {
            if (modify_group(token, username, 1) < 0) {
                free(group_copy);
                return 6;
            }
            if (verbose >= 1) {
                printf("User '%s' added to group '%s'\n", username, token);
            }
            token = strtok_r(NULL, ",", &saveptr);
        }

        free(group_copy);
        return 0;
    }

    fprintf(stderr, "usermod: no operation specified\n");
    print_usage(argv[0]);
    return 1;
}
