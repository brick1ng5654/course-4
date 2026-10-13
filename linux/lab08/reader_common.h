#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <unistd.h>

enum { REQUEST = 1, REPLY, DONE, FINISHED };
struct packet {
    int kind, from, to, reply_q;
    uint64_t time_ns;
};
struct message {
    long type;
    struct packet data;
};

static int local_q = -1, common_q = -1;

static uint64_t now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void cleanup(void)
{
    if (local_q != -1)
        msgctl(local_q, IPC_RMID, NULL);
}

static void fail(const char *where)
{
    perror(where);
    exit(EXIT_FAILURE);
}

static void send_packet(int queue, long type, int kind,
                        int to, uint64_t time_ns)
{
    struct message msg = {
        .type = type,
        .data = {kind, PROGRAM_ID, to, local_q, time_ns}
    };
    while (msgsnd(queue, &msg, sizeof(msg.data), 0) == -1) {
        if (errno != EINTR)
            fail("msgsnd");
    }
}

static int receive(int queue, long type, struct packet *data)
{
    struct message msg;
    ssize_t size = msgrcv(queue, &msg, sizeof(msg.data),
                          type, IPC_NOWAIT);
    if (size == -1) {
        if (errno == ENOMSG || errno == EINTR)
            return 0;
        fail("msgrcv");
    }
    if (size != (ssize_t)sizeof(msg.data)) {
        errno = EPROTO;
        fail("message size");
    }
    *data = msg.data;
    return 1;
}

static void show_file(const char *path, uint64_t replies[4])
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
        fail("fopen");
    printf("Program %d: replies received\n", PROGRAM_ID);
    for (int peer = 1; peer <= 3; ++peer) {
        if (peer != PROGRAM_ID)
            printf("From %d: %llu ns\n", peer,
                   (unsigned long long)replies[peer]);
    }
    printf("Program %d: BEGIN FILE\n", PROGRAM_ID);
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (putchar(ch) == EOF)
            fail("putchar");
    }
    if (ferror(file))
        fail("fgetc");
    if (fclose(file) == EOF)
        fail("fclose");
    printf("\nProgram %d: END FILE\n", PROGRAM_ID);
    if (fflush(stdout) == EOF)
        fail("fflush");
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.txt ipc.key\n", argv[0]);
        return EXIT_FAILURE;
    }
    setbuf(stdout, NULL);
    key_t key = ftok(argv[2], 'R');
    if (key == (key_t)-1)
        fail("ftok");
    common_q = msgget(key, IPC_CREAT | 0600);
    if (common_q == -1)
        fail("msgget common");
    local_q = msgget(IPC_PRIVATE, 0600);
    if (local_q == -1)
        fail("msgget local");
    if (atexit(cleanup) != 0) {
        cleanup();
        return EXIT_FAILURE;
    }

    int peer_q[4] = {-1, -1, -1, -1};
    int deferred[4] = {0}, done[4] = {0};
    int finished[4] = {0}, got_reply[4] = {0};
    uint64_t replies[4] = {0};
    uint64_t request_time = now_ns();
    int have_read = 0, reply_count = 0;
    printf("Program %d: PID=%ld common=%d local=%d\n",
           PROGRAM_ID, (long)getpid(), common_q, local_q);
    printf("Request time: %llu ns\n",
           (unsigned long long)request_time);
    for (int peer = 1; peer <= 3; ++peer) {
        if (peer != PROGRAM_ID)
            send_packet(common_q, peer, REQUEST, peer, request_time);
    }

    for (;;) {
        struct packet msg;
        while (receive(common_q, PROGRAM_ID, &msg)) {
            int peer = msg.from;
            if (peer < 1 || peer > 3 || peer == PROGRAM_ID ||
                msg.to != PROGRAM_ID) {
                errno = EPROTO;
                fail("sender or recipient");
            }
            if (msg.kind == REQUEST) {
                peer_q[peer] = msg.reply_q;
                int earlier = msg.time_ns < request_time ||
                    (msg.time_ns == request_time && peer < PROGRAM_ID);
                if (have_read || earlier) {
                    send_packet(msg.reply_q, 1, REPLY, peer, now_ns());
                    printf("Reply sent to %d\n", peer);
                } else {
                    deferred[peer] = 1;
                    printf("Request from %d deferred\n", peer);
                }
            } else if (msg.kind == DONE) {
                done[peer] = 1;
            } else {
                errno = EPROTO;
                fail("common message kind");
            }
        }
        while (receive(local_q, 1, &msg)) {
            int peer = msg.from;
            if (peer < 1 || peer > 3 || peer == PROGRAM_ID ||
                msg.to != PROGRAM_ID) {
                errno = EPROTO;
                fail("local sender or recipient");
            }
            if (msg.kind == REPLY && !got_reply[peer]) {
                got_reply[peer] = 1;
                replies[peer] = msg.time_ns;
                ++reply_count;
            } else if (msg.kind == FINISHED && PROGRAM_ID == 1) {
                finished[peer] = 1;
            }
        }
        if (!have_read && reply_count == 2) {
            show_file(argv[1], replies);
            have_read = 1;
            done[PROGRAM_ID] = 1;
            for (int peer = 1; peer <= 3; ++peer) {
                if (deferred[peer]) {
                    send_packet(peer_q[peer], 1, REPLY, peer, now_ns());
                    deferred[peer] = 0;
                    printf("Deferred reply sent to %d\n", peer);
                }
                if (peer != PROGRAM_ID)
                    send_packet(common_q, peer, DONE, peer, now_ns());
            }
        }
        if (done[1] && done[2] && done[3]) {
            if (PROGRAM_ID != 1) {
                send_packet(peer_q[1], 1, FINISHED, 1, now_ns());
                break;
            }
            if (finished[2] && finished[3]) {
                if (msgctl(common_q, IPC_RMID, NULL) == -1)
                    fail("msgctl common");
                puts("Common queue removed");
                break;
            }
        }
        if (now_ns() - request_time > 120000000000ULL) {
            fprintf(stderr, "Timeout: start all three programs\n");
            return EXIT_FAILURE;
        }
        struct timespec pause = {0, 10000000};
        nanosleep(&pause, NULL);
    }
    printf("Program %d: finished\n", PROGRAM_ID);
    return EXIT_SUCCESS;
}
