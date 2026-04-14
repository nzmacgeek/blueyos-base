/*
 * less - pager with forward and backward scrolling
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

#define MAX_LINES 100000

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

static int read_key(int tty_fd) {
    unsigned char buf[4];
    int n = (int)read(tty_fd, buf, 1);
    if (n <= 0) return -1;
    /* Handle escape sequences (arrow keys) */
    if (buf[0] == 27) {
        n = (int)read(tty_fd, buf + 1, 1);
        if (n > 0 && buf[1] == '[') {
            n = (int)read(tty_fd, buf + 2, 1);
            if (n > 0) {
                if (buf[2] == 'A') return 'k'; /* up arrow = up */
                if (buf[2] == 'B') return 'j'; /* down arrow = down */
                if (buf[2] == '5') { read(tty_fd, buf+3, 1); return 'b'; } /* PgUp = backward */
                if (buf[2] == '6') { read(tty_fd, buf+3, 1); return 'f'; } /* PgDn = forward */
            }
        }
        return 27;
    }
    return (int)buf[0];
}

static void display_lines(char **lines, int total, int top, int page_size) {
    /* Clear screen and home cursor */
    printf("\033[H\033[2J");
    int end = top + page_size;
    if (end > total) end = total;
    for (int i = top; i < end; i++) {
        fputs(lines[i], stdout);
    }
    fflush(stdout);
}

static void view_lines(char **lines, int total) {
    int tty_fd = open("/dev/tty", 0);
    if (tty_fd < 0) {
        /* No tty, just dump */
        for (int i = 0; i < total; i++) fputs(lines[i], stdout);
        return;
    }

    struct termios old, raw;
    tcgetattr(tty_fd, &old);
    raw = old;
    raw.c_lflag &= ~(unsigned)(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(tty_fd, TCSANOW, &raw);

    int page_size = get_terminal_rows();
    int top = 0;

    display_lines(lines, total, top, page_size);

    while (1) {
        /* Status bar */
        int pct = (total > 0) ? ((top + page_size) * 100 / total) : 100;
        if (pct > 100) pct = 100;
        fprintf(stderr, "\033[%dm-- line %d/%d (%d%%) --\033[0m",
                7, top + 1, total, pct);

        int ch = read_key(tty_fd);

        /* Clear status */
        fprintf(stderr, "\r                              \r");

        if (ch == 'q' || ch == 'Q') break;

        if (ch == 'j' || ch == '\n' || ch == '\r') {
            if (top + page_size < total) top++;
        } else if (ch == 'k') {
            if (top > 0) top--;
        } else if (ch == ' ' || ch == 'f') {
            top += page_size;
            if (top + page_size > total) top = total - page_size;
            if (top < 0) top = 0;
        } else if (ch == 'b') {
            top -= page_size;
            if (top < 0) top = 0;
        } else if (ch == 'g') {
            top = 0;
        } else if (ch == 'G') {
            top = total - page_size;
            if (top < 0) top = 0;
        }

        display_lines(lines, total, top, page_size);
    }

    /* Restore terminal */
    tcsetattr(tty_fd, TCSANOW, &old);
    close(tty_fd);

    /* Clear screen on exit */
    printf("\033[H\033[2J");
}

static void read_and_view(FILE *fp) {
    char **lines = malloc(MAX_LINES * sizeof(char *));
    if (!lines) {
        fprintf(stderr, "less: out of memory\n");
        return;
    }
    int count = 0;
    char buf[4096];
    while (count < MAX_LINES && fgets(buf, sizeof(buf), fp)) {
        lines[count] = strdup(buf);
        if (!lines[count]) {
            fprintf(stderr, "less: out of memory\n");
            break;
        }
        count++;
    }
    view_lines(lines, count);
    for (int i = 0; i < count; i++) free(lines[i]);
    free(lines);
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
            printf("less version %s\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "less: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        read_and_view(stdin);
    } else {
        for (int i = 0; i < file_count; i++) {
            FILE *fp = fopen(files[i], "r");
            if (!fp) {
                fprintf(stderr, "less: %s: %s\n", files[i], strerror(errno));
                continue;
            }
            read_and_view(fp);
            fclose(fp);
        }
    }

    return 0;
}
