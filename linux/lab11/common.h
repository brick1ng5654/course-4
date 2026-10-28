#ifndef COMMON_H
#define COMMON_H
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define COUNT 10
#define PORT 3434

static int timeout_arg(const char *text)
{
    char *end;
    long value = strtol(text, &end, 10);
    if (*text == '\0' || *end || value < 1 || value > 3600) {
        fprintf(stderr, "Timeout must be 1..3600 seconds\n");
        exit(EXIT_FAILURE);
    }
    return (int)value;
}

static int64_t now_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int wait_ready(int fd, short event, int64_t deadline)
{
    struct pollfd p = {.fd = fd, .events = event};
    for (;;) {
        int64_t left = deadline - now_ms();
        if (left <= 0) { errno = ETIMEDOUT; return -1; }
        int result = poll(&p, 1, (int)left);
        if (result > 0) {
            if (p.revents & POLLNVAL) {
                errno = EBADF;
                return -1;
            }
            return 0;
        }
        if (result == 0) { errno = ETIMEDOUT; return -1; }
        if (errno != EINTR) return -1;
    }
}

static int transfer(int fd, void *data, size_t size,
                    int sending, int seconds)
{
    char *p = data;
    int64_t deadline = now_ms() + seconds * 1000;
    while (size > 0) {
        if (wait_ready(fd, sending ? POLLOUT : POLLIN,
                       deadline) < 0) return -1;
        ssize_t n = sending
            ? send(fd, p, size, MSG_DONTWAIT | MSG_NOSIGNAL)
            : recv(fd, p, size, MSG_DONTWAIT);
        if (n > 0) { p += n; size -= (size_t)n; }
        else if (n == 0) { errno = ECONNRESET; return -1; }
        else if (errno != EINTR && errno != EAGAIN &&
                 errno != EWOULDBLOCK) return -1;
    }
    return 0;
}

static void print_numbers(const char *label, uint32_t *a)
{
    char line[256];
    int used = snprintf(line, sizeof(line), "PID %ld %s:",
                        (long)getpid(), label);
    for (int i = 0; i < COUNT; ++i)
        used += snprintf(line + used, sizeof(line) - (size_t)used,
                         " %u", (unsigned)a[i]);
    puts(line);
}
#endif
