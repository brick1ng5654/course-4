#ifndef IPC_H
#define IPC_H
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static inline void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static inline int number(const char *text)
{
    char *end;
    errno = 0;
    long n = strtol(text, &end, 10);
    if (errno || *text == '\0' || *end || n < 0 || n > INT_MAX) {
        fprintf(stderr, "Invalid number: %s\n", text);
        exit(EXIT_FAILURE);
    }
    return (int)n;
}

static inline void pause_ms(int ms)
{
    struct timespec t = {ms / 1000, (ms % 1000) * 1000000L};
    while (nanosleep(&t, &t) == -1)
        if (errno != EINTR) die("nanosleep");
}

static inline void operations(int id, struct sembuf *ops, size_t n)
{
    while (semop(id, ops, n) == -1)
        if (errno != EINTR) die("semop");
}

#endif
