#ifndef LAB9_COMMON_H
#define LAB9_COMMON_H
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

struct Shared {
    int lock;
    unsigned long total;
};

#if __GCC_ATOMIC_INT_LOCK_FREE != 2
#error "Lock-free int atomics are required"
#endif

static int parse_positive(const char *text, unsigned int *value)
{
    char *end;
    errno = 0;
    unsigned long n = strtoul(text, &end, 10);
    if (errno || text[0] < '0' || text[0] > '9' ||
        *end != '\0' || n == 0 || n > UINT_MAX) return -1;
    *value = (unsigned int)n;
    return 0;
}

static void delay_ms(unsigned int ms)
{
    struct timespec delay = {
        .tv_sec = ms / 1000,
        .tv_nsec = (long)(ms % 1000) * 1000000L
    };
    while (nanosleep(&delay, &delay) < 0 && errno == EINTR) {}
}

static void enter(struct Shared *state)
{
    while (__atomic_exchange_n(&state->lock, 1,
                              __ATOMIC_ACQUIRE) != 0)
        delay_ms(1);
}

static void leave(struct Shared *state)
{
    __atomic_store_n(&state->lock, 0, __ATOMIC_RELEASE);
}

static int write_all(int fd, const char *data, size_t size)
{
    while (size != 0) {
        ssize_t n = write(fd, data, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        data += n;
        size -= (size_t)n;
    }
    return 0;
}

static int cleanup(key_t key)
{
    int id = shmget(key, sizeof(struct Shared), 0600);
    if (id < 0) {
        if (errno == ENOENT) {
            puts("Shared segment does not exist.");
            return EXIT_SUCCESS;
        }
        perror("shmget");
        return EXIT_FAILURE;
    }
    struct shmid_ds info;
    if (shmctl(id, IPC_STAT, &info) < 0) {
        perror("shmctl IPC_STAT");
        return EXIT_FAILURE;
    }
    if (info.shm_nattch != 0) {
        fputs("Programs are still attached. Cleanup refused.\n",
              stderr);
        return EXIT_FAILURE;
    }
    if (shmctl(id, IPC_RMID, NULL) < 0) {
        perror("shmctl IPC_RMID");
        return EXIT_FAILURE;
    }
    printf("Removed shared segment: shmid=%d\n", id);
    return EXIT_SUCCESS;
}

static int run_writer(int number, int argc, char **argv)
{
    int cleaning = argc == 2 && strcmp(argv[1], "--cleanup") == 0;
    unsigned int interval = 0, count = 0;
    if (!cleaning && (argc != 3 ||
        parse_positive(argv[1], &interval) < 0 ||
        parse_positive(argv[2], &count) < 0)) {
        fprintf(stderr, "Usage: %s INTERVAL_MS LINE_COUNT\n"
                "       %s --cleanup\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }
    key_t key = ftok("lab9.key", '9');
    if (key == (key_t)-1) {
        perror("ftok: create lab9.key first");
        return EXIT_FAILURE;
    }
    if (cleaning) return cleanup(key);
    int id = shmget(key, sizeof(struct Shared), IPC_CREAT | 0600);
    if (id < 0) { perror("shmget"); return EXIT_FAILURE; }
    struct Shared *state = shmat(id, NULL, 0);
    if (state == (void *)-1) {
        perror("shmat");
        return EXIT_FAILURE;
    }
    int fd = open("shared_output.txt",
                  O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("open");
        shmdt(state);
        return EXIT_FAILURE;
    }
    printf("Program %d: pid=%ld, shmid=%d, interval=%u ms, "
           "count=%u\n", number, (long)getpid(), id, interval, count);
    fflush(stdout);
    int result = EXIT_SUCCESS;
    for (unsigned int i = 0; i < count; ++i) {
        char line[160];
        enter(state);
        unsigned long serial = state->total + 1;
        int length = snprintf(line, sizeof(line),
            "%lu: Program %d, line %u/%u, pid=%ld\n",
            serial, number, i + 1, count, (long)getpid());
        int error = length < 0 || (size_t)length >= sizeof(line);
        if (!error) error = write_all(fd, line, (size_t)length) < 0;
        if (!error) state->total = serial;
        leave(state);
        if (error) {
            fputs("Cannot write a complete line.\n", stderr);
            result = EXIT_FAILURE;
            break;
        }
        fputs(line, stdout);
        fflush(stdout);
        if (i + 1 < count) delay_ms(interval);
    }
    if (close(fd) < 0) { perror("close"); result = EXIT_FAILURE; }
    if (shmdt(state) < 0) { perror("shmdt"); result = EXIT_FAILURE; }
    printf("Program %d finished: %s\n", number,
           result == EXIT_SUCCESS ? "OK" : "ERROR");
    return result;
}
#endif
