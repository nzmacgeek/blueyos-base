/*
 * getent - query NSS databases
 * Part of BlueyOS base utilities
 *
 * In BlueyOS (musl libc) there is no plugin-based NSS framework.
 * The standard POSIX functions read /etc/passwd, /etc/group,
 * /etc/hosts, /etc/services, and /etc/protocols directly.
 * getent calls those functions and formats their output in the
 * traditional shadow-utils/glibc getent style.
 *
 * Supported databases: passwd, group, hosts, services, protocols
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [OPTIONS] DATABASE [KEY...]\n", prog);
    fprintf(stderr, "\n");
    fprintf(stderr, "Query NSS databases (reads /etc files directly via musl libc).\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Supported databases:\n");
    fprintf(stderr, "  passwd      User account information (/etc/passwd)\n");
    fprintf(stderr, "  group       Group information (/etc/group)\n");
    fprintf(stderr, "  hosts       Hostname-to-address mappings (/etc/hosts)\n");
    fprintf(stderr, "  services    Network services (/etc/services)\n");
    fprintf(stderr, "  protocols   Network protocols (/etc/protocols)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "If KEY is omitted all entries in the database are printed.\n");
    fprintf(stderr, "KEY may be a name or numeric id (UID, GID, port number, etc.).\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose   Verbose output\n");
    fprintf(stderr, "  --version       Print version\n");
    fprintf(stderr, "  -h, --help      Show this help\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Exit codes: 0 success, 1 usage/system error, 2 key not found\n");
}

/* ------------------------------------------------------------------ */
/* passwd                                                              */
/* ------------------------------------------------------------------ */

static void print_passwd(const struct passwd *pw) {
    printf("%s:%s:%u:%u:%s:%s:%s\n",
           pw->pw_name,
           pw->pw_passwd,
           (unsigned)pw->pw_uid,
           (unsigned)pw->pw_gid,
           pw->pw_gecos  ? pw->pw_gecos  : "",
           pw->pw_dir    ? pw->pw_dir    : "",
           pw->pw_shell  ? pw->pw_shell  : "");
}

static int do_passwd(int nkeys, char **keys) {
    int rc = 0;
    if (nkeys == 0) {
        struct passwd *pw;
        setpwent();
        while ((pw = getpwent()) != NULL)
            print_passwd(pw);
        endpwent();
        return 0;
    }
    for (int i = 0; i < nkeys; i++) {
        struct passwd *pw = NULL;
        char *end;
        unsigned long uid = strtoul(keys[i], &end, 10);
        if (*end == '\0' && end != keys[i])
            pw = getpwuid((uid_t)uid);
        else
            pw = getpwnam(keys[i]);
        if (pw) {
            print_passwd(pw);
        } else {
            if (verbose >= 1)
                fprintf(stderr, "getent: passwd: no entry for '%s'\n", keys[i]);
            rc = 2;
        }
    }
    return rc;
}

/* ------------------------------------------------------------------ */
/* group                                                               */
/* ------------------------------------------------------------------ */

static void print_group(const struct group *gr) {
    printf("%s:%s:%u:", gr->gr_name, gr->gr_passwd, (unsigned)gr->gr_gid);
    if (gr->gr_mem) {
        for (int i = 0; gr->gr_mem[i]; i++) {
            if (i > 0)
                putchar(',');
            fputs(gr->gr_mem[i], stdout);
        }
    }
    putchar('\n');
}

static int do_group(int nkeys, char **keys) {
    int rc = 0;
    if (nkeys == 0) {
        struct group *gr;
        setgrent();
        while ((gr = getgrent()) != NULL)
            print_group(gr);
        endgrent();
        return 0;
    }
    for (int i = 0; i < nkeys; i++) {
        struct group *gr = NULL;
        char *end;
        unsigned long gid = strtoul(keys[i], &end, 10);
        if (*end == '\0' && end != keys[i])
            gr = getgrgid((gid_t)gid);
        else
            gr = getgrnam(keys[i]);
        if (gr) {
            print_group(gr);
        } else {
            if (verbose >= 1)
                fprintf(stderr, "getent: group: no entry for '%s'\n", keys[i]);
            rc = 2;
        }
    }
    return rc;
}

/* ------------------------------------------------------------------ */
/* hosts                                                               */
/* ------------------------------------------------------------------ */

static void print_host(const struct hostent *he) {
    char addrstr[INET6_ADDRSTRLEN];
    for (int i = 0; he->h_addr_list[i]; i++) {
        if (he->h_addrtype == AF_INET) {
            struct in_addr in;
            memcpy(&in, he->h_addr_list[i], sizeof(in));
            if (!inet_ntop(AF_INET, &in, addrstr, sizeof(addrstr)))
                continue;
        } else if (he->h_addrtype == AF_INET6) {
            struct in6_addr in6;
            memcpy(&in6, he->h_addr_list[i], sizeof(in6));
            if (!inet_ntop(AF_INET6, &in6, addrstr, sizeof(addrstr)))
                continue;
        } else {
            continue;
        }
        printf("%-16s %s", addrstr, he->h_name);
        if (he->h_aliases) {
            for (int j = 0; he->h_aliases[j]; j++)
                printf(" %s", he->h_aliases[j]);
        }
        putchar('\n');
    }
}

static int do_hosts(int nkeys, char **keys) {
    int rc = 0;
    if (nkeys == 0) {
        /* Enumerate by reading /etc/hosts directly — gethostent() is not
         * available in all musl versions and is not needed here. */
        FILE *fp = fopen("/etc/hosts", "r");
        if (!fp) {
            fprintf(stderr, "getent: hosts: cannot open /etc/hosts: %s\n",
                    strerror(errno));
            return 1;
        }
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            char *hash = strchr(line, '#');
            if (hash)
                *hash = '\0';
            char *p = line;
            while (*p == ' ' || *p == '\t')
                p++;
            if (*p && *p != '\n' && *p != '\r')
                fputs(line, stdout);
        }
        fclose(fp);
        return 0;
    }
    for (int i = 0; i < nkeys; i++) {
        struct hostent *he = gethostbyname(keys[i]);
        if (he) {
            print_host(he);
        } else {
            if (verbose >= 1)
                fprintf(stderr, "getent: hosts: no entry for '%s'\n", keys[i]);
            rc = 2;
        }
    }
    return rc;
}

/* ------------------------------------------------------------------ */
/* services                                                            */
/* ------------------------------------------------------------------ */

static void print_service(const struct servent *se) {
    printf("%-20s %d/%s", se->s_name, ntohs((uint16_t)se->s_port), se->s_proto);
    if (se->s_aliases) {
        for (int i = 0; se->s_aliases[i]; i++)
            printf(" %s", se->s_aliases[i]);
    }
    putchar('\n');
}

static int do_services(int nkeys, char **keys) {
    int rc = 0;
    if (nkeys == 0) {
        struct servent *se;
        setservent(0);
        while ((se = getservent()) != NULL)
            print_service(se);
        endservent();
        return 0;
    }
    for (int i = 0; i < nkeys; i++) {
        struct servent *se = NULL;
        /* Key may be "name", "port/proto", or bare "port" */
        char keybuf[256];
        strncpy(keybuf, keys[i], sizeof(keybuf) - 1);
        keybuf[sizeof(keybuf) - 1] = '\0';

        char *slash = strchr(keybuf, '/');
        const char *proto = NULL;
        if (slash) {
            *slash = '\0';
            proto = slash + 1;
        }
        char *end;
        unsigned long port = strtoul(keybuf, &end, 10);
        if (*end == '\0' && end != keybuf)
            se = getservbyport((int)htons((uint16_t)port), proto);
        else
            se = getservbyname(keybuf, proto);
        if (se) {
            print_service(se);
        } else {
            if (verbose >= 1)
                fprintf(stderr, "getent: services: no entry for '%s'\n", keys[i]);
            rc = 2;
        }
    }
    return rc;
}

/* ------------------------------------------------------------------ */
/* protocols                                                           */
/* ------------------------------------------------------------------ */

static void print_protocol(const struct protoent *pe) {
    printf("%-20s %d", pe->p_name, pe->p_proto);
    if (pe->p_aliases) {
        for (int i = 0; pe->p_aliases[i]; i++)
            printf(" %s", pe->p_aliases[i]);
    }
    putchar('\n');
}

static int do_protocols(int nkeys, char **keys) {
    int rc = 0;
    if (nkeys == 0) {
        struct protoent *pe;
        setprotoent(0);
        while ((pe = getprotoent()) != NULL)
            print_protocol(pe);
        endprotoent();
        return 0;
    }
    for (int i = 0; i < nkeys; i++) {
        struct protoent *pe = NULL;
        char *end;
        unsigned long num = strtoul(keys[i], &end, 10);
        if (*end == '\0' && end != keys[i])
            pe = getprotobynumber((int)num);
        else
            pe = getprotobyname(keys[i]);
        if (pe) {
            print_protocol(pe);
        } else {
            if (verbose >= 1)
                fprintf(stderr, "getent: protocols: no entry for '%s'\n", keys[i]);
            rc = 2;
        }
    }
    return rc;
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env)
        verbose = atoi(verbose_env);

    /* --version / --help before anything else (no root required) */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("getent version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    /* Parse options that precede the database name */
    int opt_end = 1;
    for (; opt_end < argc; opt_end++) {
        if (strcmp(argv[opt_end], "-v") == 0 || strcmp(argv[opt_end], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[opt_end][0] == '-') {
            fprintf(stderr, "getent: unknown option '%s'\n", argv[opt_end]);
            return 1;
        } else {
            break;
        }
    }

    if (opt_end >= argc) {
        fprintf(stderr, "getent: missing database argument\n");
        print_usage(argv[0]);
        return 1;
    }

    const char *db    = argv[opt_end];
    char      **keys  = &argv[opt_end + 1];
    int         nkeys = argc - opt_end - 1;

    if (verbose >= 2)
        fprintf(stderr, "getent: querying database '%s' with %d key(s)\n", db, nkeys);

    if (strcmp(db, "passwd") == 0)
        return do_passwd(nkeys, keys);
    if (strcmp(db, "group") == 0)
        return do_group(nkeys, keys);
    if (strcmp(db, "hosts") == 0)
        return do_hosts(nkeys, keys);
    if (strcmp(db, "services") == 0)
        return do_services(nkeys, keys);
    if (strcmp(db, "protocols") == 0)
        return do_protocols(nkeys, keys);

    fprintf(stderr, "getent: unknown database '%s'\n", db);
    fprintf(stderr, "Supported: passwd, group, hosts, services, protocols\n");
    return 1;
}
