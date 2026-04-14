/*
 * more - simple file pager
 * Part of BlueyOS base utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static int verbose = 0;

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose    Verbose output\n");
    fprintf(stderr, "  --version        Print version\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

static int get_terminal_rows(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 2) {
        return ws.ws_row - 1;
    }
    return 23;
}

static int read_key(void) {
    int tty_fd = open("/dev/tty", 0); /* O_RDONLY=0 */
    if (tty_fd < 0) return '\n';

    struct termios old, raw;
    tcgetattr(tty_fd, &old);
    raw = old;
    raw.c_lflag &= ~(unsigned)(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(tty_fd, TCSANOW, &raw);

    unsigned char ch;
    int n = (int)read(tty_fd, &ch, 1);

    tcsetattr(tty_fd, TCSANOW, &old);
    close(tty_fd);

    return (n > 0) ? (int)ch : -1;
}

static void page_file(FILE *fp, const char *name) {
    int page_rows = get_terminal_rows();
    char line[4096];
    int line_count = 0;
    long bytes_read = 0;
    long total_bytes = 0;

    /* Get file size for percentage */
    long start = ftell(fp);
    fseek(fp, 0, SEEK_END);
    total_bytes = ftell(fp);
    fseek(fp, start, SEEK_SET);

    while (fgets(line, sizeof(line), fp)) {
        fputs(line, stdout);
        bytes_read += (long)strlen(line);
        line_count++;

        if (line_count >= page_rows) {
            /* Print --More-- prompt */
            int pct = (total_bytes > 0) ? (int)(bytes_read * 100 / total_bytes) : 0;
            if (name) {
                fprintf(stderr, "--More--(%d%%) [%s]", pct, name);
            } else {
                fprintf(stderr, "--More--");
            }

            int ch = read_key();
            fprintf(stderr, "\r                                \r");

            if (ch == 'q' || ch == 'Q') {
                return;
            }
            /* Any other key advances one page */
            line_count = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    const char *verbose_env = getenv("VERBOSE");
    if (verbose_env) {
        verbose = atoi(verbose_env);
    }

    char *files[256];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("more version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "more: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        page_file(stdin, NULL);
    } else {
        for (int i = 0; i < file_count; i++) {
            FILE *fp = fopen(files[i], "r");
            if (!fp) {
                fprintf(stderr, "more: %s: %s\n", files[i], strerror(errno));
                continue;
            }
            if (file_count > 1) {
                printf(":::::::::::::::\n%s\n:::::::::::::::\n", files[i]);
            }
            page_file(fp, files[i]);
            fclose(fp);
        }
    }

    return 0;
}
