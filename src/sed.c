/*
 * sed - stream editor
 * Part of BlueyOS base utilities
 *
 * A simplified implementation of the UNIX sed command
 * Supporting basic substitution and deletion
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif
#define MAX_LINE 8192
#define MAX_COMMANDS 64

static int verbose = 0;
static int in_place = 0;
static int quiet = 0;

typedef enum {
    CMD_SUBSTITUTE,
    CMD_DELETE,
    CMD_PRINT
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    char *pattern;
    char *replacement;
    regex_t regex;
    int global;
    int compiled;
} sed_command_t;

static sed_command_t commands[MAX_COMMANDS];
static int num_commands = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] 'SCRIPT' [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n              Suppress automatic printing\n");
    fprintf(stderr, "  -i              Edit files in-place\n");
    fprintf(stderr, "  -e SCRIPT       Add script to commands\n");
    fprintf(stderr, "  --verbose       Verbose output\n");
    fprintf(stderr, "  --version       Print version\n");
    fprintf(stderr, "  --help          Show this help\n");
    fprintf(stderr, "\nSupported commands:\n");
    fprintf(stderr, "  s/PATTERN/REPLACEMENT/[g]  Substitute\n");
    fprintf(stderr, "  d                           Delete line\n");
    fprintf(stderr, "  p                           Print line\n");
}

static int parse_substitute_command(const char *cmd, sed_command_t *sc) {
    char delim;
    const char *p;
    char pattern[512], replacement[512];
    int pi = 0, ri = 0;
    int state = 0; /* 0=pattern, 1=replacement, 2=flags */

    if (cmd[0] != 's') {
        return -1;
    }

    delim = cmd[1];
    if (delim == '\0') {
        return -1;
    }

    p = cmd + 2;
    while (*p) {
        if (*p == delim) {
            state++;
            if (state > 2) break;
            p++;
            continue;
        }

        if (state == 0) {
            /* Pattern */
            if (pi < sizeof(pattern) - 1) {
                pattern[pi++] = *p;
            }
        } else if (state == 1) {
            /* Replacement */
            if (ri < sizeof(replacement) - 1) {
                replacement[ri++] = *p;
            }
        } else if (state == 2) {
            /* Flags */
            if (*p == 'g') {
                sc->global = 1;
            }
        }
        p++;
    }

    pattern[pi] = '\0';
    replacement[ri] = '\0';

    if (state < 1) {
        return -1; /* Incomplete command */
    }

    sc->type = CMD_SUBSTITUTE;
    sc->pattern = strdup(pattern);
    sc->replacement = strdup(replacement);
    sc->global = (state >= 2) && sc->global;

    /* Compile regex */
    int ret = regcomp(&sc->regex, pattern, REG_EXTENDED);
    if (ret != 0) {
        char errbuf[256];
        regerror(ret, &sc->regex, errbuf, sizeof(errbuf));
        fprintf(stderr, "sed: invalid pattern: %s\n", errbuf);
        return -1;
    }
    sc->compiled = 1;

    return 0;
}

static int parse_command(const char *cmd) {
    sed_command_t *sc;

    if (num_commands >= MAX_COMMANDS) {
        fprintf(stderr, "sed: too many commands\n");
        return -1;
    }

    sc = &commands[num_commands];
    memset(sc, 0, sizeof(*sc));

    if (cmd[0] == 's') {
        if (parse_substitute_command(cmd, sc) < 0) {
            return -1;
        }
    } else if (cmd[0] == 'd' && (cmd[1] == '\0' || cmd[1] == ';')) {
        sc->type = CMD_DELETE;
    } else if (cmd[0] == 'p' && (cmd[1] == '\0' || cmd[1] == ';')) {
        sc->type = CMD_PRINT;
    } else {
        fprintf(stderr, "sed: unknown command '%c'\n", cmd[0]);
        return -1;
    }

    num_commands++;
    return 0;
}

static char *apply_substitution(const char *line, sed_command_t *sc) {
    regmatch_t match;
    char result[MAX_LINE * 2];
    char *output = NULL;
    const char *p = line;
    int result_len = 0;
    int matched = 0;

    result[0] = '\0';

    while (*p) {
        if (regexec(&sc->regex, p, 1, &match, 0) == 0) {
            matched = 1;

            /* Copy prefix */
            int prefix_len = match.rm_so;
            if (result_len + prefix_len < sizeof(result) - 1) {
                memcpy(result + result_len, p, prefix_len);
                result_len += prefix_len;
            }

            /* Add replacement */
            int repl_len = strlen(sc->replacement);
            if (result_len + repl_len < sizeof(result) - 1) {
                memcpy(result + result_len, sc->replacement, repl_len);
                result_len += repl_len;
            }

            /* Move past match */
            p += match.rm_eo;

            /* If not global, copy rest and break */
            if (!sc->global) {
                int rest_len = strlen(p);
                if (result_len + rest_len < sizeof(result) - 1) {
                    memcpy(result + result_len, p, rest_len);
                    result_len += rest_len;
                }
                break;
            }
        } else {
            /* No more matches, copy rest */
            int rest_len = strlen(p);
            if (result_len + rest_len < sizeof(result) - 1) {
                memcpy(result + result_len, p, rest_len);
                result_len += rest_len;
            }
            break;
        }
    }

    result[result_len] = '\0';

    if (matched) {
        output = strdup(result);
    } else {
        output = strdup(line);
    }

    return output;
}

static int process_file(FILE *in, FILE *out) {
    char line[MAX_LINE];
    int line_num = 0;

    while (fgets(line, sizeof(line), in) != NULL) {
        line_num++;
        int delete_line = 0;
        int print_line = 0;
        char *current = strdup(line);
        char *next;

        /* Remove trailing newline */
        size_t len = strlen(current);
        if (len > 0 && current[len - 1] == '\n') {
            current[len - 1] = '\0';
        }

        /* Apply commands */
        for (int i = 0; i < num_commands; i++) {
            sed_command_t *sc = &commands[i];

            switch (sc->type) {
                case CMD_SUBSTITUTE:
                    next = apply_substitution(current, sc);
                    free(current);
                    current = next;
                    break;

                case CMD_DELETE:
                    delete_line = 1;
                    break;

                case CMD_PRINT:
                    print_line = 1;
                    break;
            }
        }

        /* Output line unless deleted */
        if (!delete_line) {
            if (print_line || !quiet) {
                fprintf(out, "%s\n", current);
            }
        }

        free(current);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int i;
    char *script = NULL;
    int file_count = 0;
    char *files[256];

    /* Check for VERBOSE environment variable */
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("sed version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            quiet = 1;
        } else if (strcmp(argv[i], "-i") == 0) {
            in_place = 1;
        } else if (strcmp(argv[i], "-e") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "sed: -e requires an argument\n");
                return 1;
            }
            script = argv[i];
            if (parse_command(script) < 0) {
                return 1;
            }
            script = NULL; /* Mark as processed */
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "sed: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else if (!script) {
            script = argv[i];
        } else {
            /* It's a file */
            if (file_count < 256) {
                files[file_count++] = argv[i];
            }
        }
    }

    /* Parse script if not yet processed via -e */
    if (script && num_commands == 0) {
        if (parse_command(script) < 0) {
            return 1;
        }
    }

    if (num_commands == 0) {
        fprintf(stderr, "sed: no script specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Process files or stdin */
    if (file_count == 0) {
        /* Read from stdin */
        process_file(stdin, stdout);
    } else {
        /* Process each file */
        for (i = 0; i < file_count; i++) {
            FILE *fp = fopen(files[i], "r");
            if (!fp) {
                fprintf(stderr, "sed: %s: %s\n", files[i], strerror(errno));
                continue;
            }

            if (verbose >= 2) {
                fprintf(stderr, "sed: processing file '%s'\n", files[i]);
            }

            if (in_place) {
                /* In-place editing: write to temp file then rename */
                char tmpfile[512];
                snprintf(tmpfile, sizeof(tmpfile), "%s.tmp", files[i]);

                FILE *out = fopen(tmpfile, "w");
                if (!out) {
                    fprintf(stderr, "sed: cannot create temp file: %s\n", strerror(errno));
                    fclose(fp);
                    continue;
                }

                process_file(fp, out);
                fclose(fp);
                fclose(out);

                if (rename(tmpfile, files[i]) < 0) {
                    fprintf(stderr, "sed: cannot rename temp file: %s\n", strerror(errno));
                }
            } else {
                process_file(fp, stdout);
                fclose(fp);
            }
        }
    }

    /* Clean up */
    for (i = 0; i < num_commands; i++) {
        if (commands[i].pattern) free(commands[i].pattern);
        if (commands[i].replacement) free(commands[i].replacement);
        if (commands[i].compiled) regfree(&commands[i].regex);
    }

    return 0;
}
