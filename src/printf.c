/*
 * printf - format and print data
 * Part of BlueyOS base utilities
 * POSIX-compliant printf implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s FORMAT [ARGUMENT]...\n", progname);
    fprintf(stderr, "Print ARGUMENT(s) according to FORMAT.\n\n");
    fprintf(stderr, "Format sequences:\n");
    fprintf(stderr, "  \\\\      backslash\n");
    fprintf(stderr, "  \\n      newline\n");
    fprintf(stderr, "  \\t      horizontal tab\n");
    fprintf(stderr, "  \\r      carriage return\n");
    fprintf(stderr, "  \\xHH    byte with hex value HH\n");
    fprintf(stderr, "  %%      percent sign\n");
    fprintf(stderr, "  %%s      string\n");
    fprintf(stderr, "  %%d %%i  decimal integer\n");
    fprintf(stderr, "  %%u      unsigned decimal\n");
    fprintf(stderr, "  %%x      hexadecimal\n");
    fprintf(stderr, "  %%o      octal\n");
    fprintf(stderr, "  %%c      character\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  --version    Print version\n");
    fprintf(stderr, "  --help       Show this help\n");
}

static int parse_escape(const char **fmt) {
    const char *p = *fmt;
    if (*p != '\\') return -1;
    p++;

    switch (*p) {
        case '\\': *fmt = p; return '\\';
        case 'n':  *fmt = p; return '\n';
        case 't':  *fmt = p; return '\t';
        case 'r':  *fmt = p; return '\r';
        case 'a':  *fmt = p; return '\a';
        case 'b':  *fmt = p; return '\b';
        case 'f':  *fmt = p; return '\f';
        case 'v':  *fmt = p; return '\v';
        case 'x': {
            p++;
            int val = 0;
            for (int i = 0; i < 2 && isxdigit(*p); i++, p++) {
                val = val * 16 + (isdigit(*p) ? *p - '0' :
                                  tolower(*p) - 'a' + 10);
            }
            *fmt = p - 1;
            return val;
        }
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': {
            int val = 0;
            for (int i = 0; i < 3 && *p >= '0' && *p <= '7'; i++, p++) {
                val = val * 8 + (*p - '0');
            }
            *fmt = p - 1;
            return val;
        }
        default:
            *fmt = p;
            return *p;
    }
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    if (argc < 2) {
        fprintf(stderr, "printf: missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("printf version %s\n", VERSION);
        return 0;
    } else if (strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    const char *format = argv[1];
    int arg_idx = 2;

    const char *p = format;
    while (*p) {
        if (*p == '\\') {
            int ch = parse_escape(&p);
            if (ch >= 0) putchar(ch);
            p++;
        } else if (*p == '%' && *(p + 1) != '\0') {
            p++;
            if (*p == '%') {
                putchar('%');
                p++;
            } else {
                const char *arg = (arg_idx < argc) ? argv[arg_idx++] : "";

                switch (*p) {
                    case 's':
                        fputs(arg, stdout);
                        break;
                    case 'd':
                    case 'i':
                        printf("%d", atoi(arg));
                        break;
                    case 'u':
                        printf("%u", (unsigned int)strtoul(arg, NULL, 10));
                        break;
                    case 'x':
                        printf("%x", (unsigned int)strtoul(arg, NULL, 0));
                        break;
                    case 'X':
                        printf("%X", (unsigned int)strtoul(arg, NULL, 0));
                        break;
                    case 'o':
                        printf("%o", (unsigned int)strtoul(arg, NULL, 0));
                        break;
                    case 'c':
                        if (*arg) putchar(*arg);
                        break;
                    default:
                        putchar('%');
                        putchar(*p);
                        break;
                }
                p++;
            }
        } else {
            putchar(*p);
            p++;
        }
    }

    return 0;
}
