#ifndef COMMON_H
#define COMMON_H
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static inline void fail(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static inline void write_all(int fd, const void *data, size_t size)
{
    const char *p = data;
    while (size > 0) {
        ssize_t n = write(fd, p, size);
        if (n == -1 && errno == EINTR) continue;
        if (n <= 0) fail("write");
        p += n;
        size -= (size_t)n;
    }
}

static inline int wait_child(pid_t pid)
{
    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) fail("waitpid");
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif
#ifndef COMMON_H
#define COMMON_H
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static inline void fail(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static inline void write_all(int fd, const void *data, size_t size)
{
    const char *p = data;
    while (size > 0) {
        ssize_t n = write(fd, p, size);
        if (n == -1 && errno == EINTR) continue;
        if (n <= 0) fail("write");
        p += n;
        size -= (size_t)n;
    }
}

static inline int wait_child(pid_t pid)
{
    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) fail("waitpid");
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif
