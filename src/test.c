/*
 * test - evaluate conditional expressions
 * Part of BlueyOS base utilities
 * Implements POSIX test / [ command
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s EXPRESSION\n", progname);
    fprintf(stderr, "   or: [ EXPRESSION ]\n");
    fprintf(stderr, "Evaluate conditional expression.\n\n");
    fprintf(stderr, "File tests:\n");
    fprintf(stderr, "  -e FILE     FILE exists\n");
    fprintf(stderr, "  -f FILE     FILE is a regular file\n");
    fprintf(stderr, "  -d FILE     FILE is a directory\n");
    fprintf(stderr, "  -L FILE     FILE is a symbolic link\n");
    fprintf(stderr, "  -r FILE     FILE is readable\n");
    fprintf(stderr, "  -w FILE     FILE is writable\n");
    fprintf(stderr, "  -x FILE     FILE is executable\n");
    fprintf(stderr, "  -s FILE     FILE exists and has size > 0\n");
    fprintf(stderr, "\nString tests:\n");
    fprintf(stderr, "  -z STRING   STRING is empty\n");
    fprintf(stderr, "  -n STRING   STRING is not empty\n");
    fprintf(stderr, "  STRING      STRING is not empty\n");
    fprintf(stderr, "  S1 = S2     strings are equal\n");
    fprintf(stderr, "  S1 != S2    strings are not equal\n");
    fprintf(stderr, "\nInteger tests:\n");
    fprintf(stderr, "  N1 -eq N2   N1 equals N2\n");
    fprintf(stderr, "  N1 -ne N2   N1 not equals N2\n");
    fprintf(stderr, "  N1 -lt N2   N1 less than N2\n");
    fprintf(stderr, "  N1 -le N2   N1 less than or equal N2\n");
    fprintf(stderr, "  N1 -gt N2   N1 greater than N2\n");
    fprintf(stderr, "  N1 -ge N2   N1 greater than or equal N2\n");
    fprintf(stderr, "\nLogical:\n");
    fprintf(stderr, "  ! EXPR      NOT EXPR\n");
    fprintf(stderr, "  E1 -a E2    E1 AND E2\n");
    fprintf(stderr, "  E1 -o E2    E1 OR E2\n");
    fprintf(stderr, "\nExit status is 0 if EXPRESSION is true, 1 if false, 2 on error.\n");
}

static int is_integer(const char *str) {
    if (!str || !*str) return 0;
    if (*str == '-' || *str == '+') str++;
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    }
    return 1;
}

static int test_file(const char *op, const char *path) {
    struct stat st;

    if (strcmp(op, "-e") == 0) {
        return stat(path, &st) == 0;
    } else if (strcmp(op, "-f") == 0) {
        return stat(path, &st) == 0 && S_ISREG(st.st_mode);
    } else if (strcmp(op, "-d") == 0) {
        return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
    } else if (strcmp(op, "-L") == 0) {
        return lstat(path, &st) == 0 && S_ISLNK(st.st_mode);
    } else if (strcmp(op, "-r") == 0) {
        return access(path, R_OK) == 0;
    } else if (strcmp(op, "-w") == 0) {
        return access(path, W_OK) == 0;
    } else if (strcmp(op, "-x") == 0) {
        return access(path, X_OK) == 0;
    } else if (strcmp(op, "-s") == 0) {
        return stat(path, &st) == 0 && st.st_size > 0;
    }
    return 0;
}

static int eval_error = 0;

static int evaluate(int argc, char *argv[], int start, int end) {
    if (start >= end) return 0;

    /* Single argument */
    if (end - start == 1) {
        return argv[start][0] != '\0';
    }

    /* Negation */
    if (strcmp(argv[start], "!") == 0) {
        return !evaluate(argc, argv, start + 1, end);
    }

    /* Two arguments */
    if (end - start == 2) {
        const char *op = argv[start];
        const char *arg = argv[start + 1];

        if (strcmp(op, "-z") == 0) {
            return arg[0] == '\0';
        } else if (strcmp(op, "-n") == 0) {
            return arg[0] != '\0';
        } else if (op[0] == '-') {
            /* Validate known file test operators */
            static const char *valid_file_ops[] = {
                "-e", "-f", "-d", "-L", "-r", "-w", "-x", "-s", NULL
            };
            for (int j = 0; valid_file_ops[j]; j++) {
                if (strcmp(op, valid_file_ops[j]) == 0) {
                    return test_file(op, arg);
                }
            }
            fprintf(stderr, "test: unknown unary operator '%s'\n", op);
            eval_error = 1;
            return 0;
        }
        fprintf(stderr, "test: missing binary operator\n");
        eval_error = 1;
        return 0;
    }

    /* Three arguments */
    if (end - start == 3) {
        const char *left = argv[start];
        const char *op = argv[start + 1];
        const char *right = argv[start + 2];

        /* String comparisons */
        if (strcmp(op, "=") == 0) {
            return strcmp(left, right) == 0;
        } else if (strcmp(op, "!=") == 0) {
            return strcmp(left, right) != 0;
        }

        /* Integer comparisons */
        static const char *int_ops[] = {
            "-eq", "-ne", "-lt", "-le", "-gt", "-ge", NULL
        };
        for (int j = 0; int_ops[j]; j++) {
            if (strcmp(op, int_ops[j]) == 0) {
                if (!is_integer(left) || !is_integer(right)) {
                    fprintf(stderr, "test: integer expression expected\n");
                    eval_error = 1;
                    return 0;
                }
                int l = atoi(left);
                int r = atoi(right);
                if (strcmp(op, "-eq") == 0) return l == r;
                if (strcmp(op, "-ne") == 0) return l != r;
                if (strcmp(op, "-lt") == 0) return l < r;
                if (strcmp(op, "-le") == 0) return l <= r;
                if (strcmp(op, "-gt") == 0) return l > r;
                if (strcmp(op, "-ge") == 0) return l >= r;
            }
        }

        fprintf(stderr, "test: unknown binary operator '%s'\n", op);
        eval_error = 1;
        return 0;
    }

    /* Look for -a and -o operators (left to right) */
    for (int i = start + 1; i < end - 1; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            return evaluate(argc, argv, start, i) || evaluate(argc, argv, i + 1, end);
        }
    }

    for (int i = start + 1; i < end - 1; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            return evaluate(argc, argv, start, i) && evaluate(argc, argv, i + 1, end);
        }
    }

    fprintf(stderr, "test: syntax error\n");
    eval_error = 1;
    return 0;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    /* Check if invoked as [ */
    int is_bracket = 0;
    const char *progname = strrchr(argv[0], '/');
    progname = progname ? progname + 1 : argv[0];

    if (strcmp(progname, "[") == 0) {
        is_bracket = 1;
        if (argc < 2 || strcmp(argv[argc - 1], "]") != 0) {
            fprintf(stderr, "[: missing ']'\n");
            return 2;
        }
        argc--; /* Remove the trailing ] */
    }

    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("test version %s\n", VERSION);
        return 0;
    } else if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    eval_error = 0;
    int result = evaluate(argc, argv, 1, argc);

    if (eval_error) {
        return 2;
    }

    if (verbose >= 2) {
        fprintf(stderr, "test: result = %d\n", result);
    }

    return result ? 0 : 1;
}
