#include "common.h"
#include <fcntl.h>

int main(int argc, char *argv[])
{
    if (argc != 5) return EXIT_FAILURE;
    int fd = atoi(argv[1]);
    int number = atoi(argv[2]);
    pid_t peer = (pid_t)atol(argv[3]);
    int turn = number == 1 ? SIGUSR1 : SIGUSR2;
    int next_turn = number == 1 ? SIGUSR2 : SIGUSR1;
    int output = open(argv[4], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output == -1) fail("open output");
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, turn);
    sigaddset(&signals, SIGQUIT);
    if (number == 2 && kill(peer, SIGUSR1) == -1)
        fail("kill first turn");
    int finished = 0;
    for (;;) {
        siginfo_t info;
        int signal_number;
        do {
            signal_number = sigwaitinfo(&signals, &info);
        } while (signal_number == -1 && errno == EINTR);
        if (signal_number == -1) fail("sigwaitinfo");
        if (signal_number == SIGQUIT) {
            finished = 1;
            continue;
        }
        if (peer == 0) peer = info.si_pid;
        char symbol;
        ssize_t n;
        do {
            n = read(fd, &symbol, 1);
        } while (n == -1 && errno == EINTR);
        if (n == -1) fail("read");
        if (n == 0) {
            if (!finished) {
                sigset_t quit;
                sigemptyset(&quit);
                sigaddset(&quit, SIGQUIT);
                int received;
                int error = sigwait(&quit, &received);
                if (error != 0) { errno = error; fail("sigwait"); }
            }
            if (kill(peer, next_turn) == -1 && errno != ESRCH)
                fail("kill final turn");
            break;
        }
        write_all(output, &symbol, 1);
        if (kill(peer, next_turn) == -1) fail("kill next turn");
    }
    close(fd);
    if (close(output) == -1) fail("close output");
    return EXIT_SUCCESS;
}
