/*
 * xargs - build and execute command lines from standard input
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [CMD [ARGS]]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n N    Max number of arguments per invocation\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int run_cmd(char **argv, int argc) {
    if (argc == 0) return 0;

    if (verbose >= 1) {
        fprintf(stderr, "xargs: running:");
        for (int i = 0; argv[i]; i++) fprintf(stderr, " %s", argv[i]);
        fprintf(stderr, "\n");
    }

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "xargs: fork failed: %s\n", strerror(errno));
        return -1;
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "xargs: exec '%s': %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    int max_args = 0; /* 0 = unlimited */
    int cmd_start = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("xargs version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "xargs: -n requires an argument\n");
                return 1;
            }
            max_args = atoi(argv[i]);
            if (max_args <= 0) {
                fprintf(stderr, "xargs: -n must be positive\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "xargs: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            cmd_start = i;
            break;
        }
    }

    /* Collect initial command args */
    int base_argc = 0;
    char *base_cmd[1024];
    if (cmd_start >= 0) {
        for (int i = cmd_start; i < argc && base_argc < 1023; i++) {
            base_cmd[base_argc++] = argv[i];
        }
    } else {
        base_cmd[0] = "echo";
        base_argc = 1;
    }

    /* Read stdin arguments */
    char *input_args[4096];
    int input_count = 0;
    char word[4096];
    int wi = 0;

    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (c == '\n' || c == ' ' || c == '\t') {
            if (wi > 0) {
                word[wi] = '\0';
                if (input_count < 4095) {
                    char *dup = strdup(word);
                    if (!dup) {
                        fprintf(stderr, "xargs: out of memory\n");
                        for (int j = 0; j < input_count; j++) free(input_args[j]);
                        return 1;
                    }
                    input_args[input_count++] = dup;
                }
                wi = 0;
            }
        } else {
            if (wi < (int)sizeof(word) - 1) word[wi++] = (char)c;
        }
    }
    if (wi > 0) {
        word[wi] = '\0';
        if (input_count < 4095) {
            char *dup = strdup(word);
            if (!dup) {
                fprintf(stderr, "xargs: out of memory\n");
                for (int j = 0; j < input_count; j++) free(input_args[j]);
                return 1;
            }
            input_args[input_count++] = dup;
        }
    }

    if (input_count == 0) return 0;

    int ret = 0;
    int batch = (max_args > 0) ? max_args : input_count;

    for (int i = 0; i < input_count; i += batch) {
        char *cmd[2048];
        int ci = 0;
        for (int j = 0; j < base_argc; j++) cmd[ci++] = base_cmd[j];
        int end = i + batch;
        if (end > input_count) end = input_count;
        for (int j = i; j < end; j++) cmd[ci++] = input_args[j];
        cmd[ci] = NULL;
        int r = run_cmd(cmd, ci);
        if (r != 0) ret = r;
    }

    for (int i = 0; i < input_count; i++) free(input_args[i]);
    return ret;
}
