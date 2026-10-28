#include "common.h"

static int compare(const void *left, const void *right)
{
    uint32_t a = *(const uint32_t *)left;
    uint32_t b = *(const uint32_t *)right;
    return (a > b) - (a < b);
}

static int serve(int fd, int seconds)
{
    uint32_t numbers[COUNT];
    if (transfer(fd, numbers, sizeof(numbers), 0, seconds) < 0) {
        perror("receive from client");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < COUNT; ++i)
        numbers[i] = ntohl(numbers[i]);
    qsort(numbers, COUNT, sizeof(numbers[0]), compare);
    print_numbers("sorted", numbers);
    for (int i = 0; i < COUNT; ++i)
        numbers[i] = htonl(numbers[i]);
    if (transfer(fd, numbers, sizeof(numbers), 1, seconds) < 0) {
        perror("send to client");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s timeout_seconds\n", argv[0]);
        return EXIT_FAILURE;
    }
    int seconds = timeout_arg(argv[1]);
    setvbuf(stdout, NULL, _IONBF, 0);
    struct sigaction sa = {0};
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_NOCLDWAIT;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    int listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listener < 0) { perror("socket"); return EXIT_FAILURE; }
    int one = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
                   &one, sizeof(one)) < 0) {
        perror("setsockopt"); close(listener); return EXIT_FAILURE;
    }
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, (struct sockaddr *)&address,
             sizeof(address)) < 0 || listen(listener, 16) < 0) {
        perror("bind/listen"); close(listener); return EXIT_FAILURE;
    }
    printf("Server PID %ld, port %d, timeout %d s\n",
           (long)getpid(), PORT, seconds);
    int status = EXIT_SUCCESS;
    int64_t deadline = now_ms() + seconds * 1000;
    for (;;) {
        if (wait_ready(listener, POLLIN, deadline) < 0) {
            if (errno == ETIMEDOUT)
                printf("No new clients for %d s. Server stopped.\n",
                       seconds);
            else { perror("wait client"); status = EXIT_FAILURE; }
            break;
        }
        int fd = accept(listener, NULL, NULL);
        if (fd < 0) {
            if (errno == EINTR || errno == EAGAIN ||
                errno == EWOULDBLOCK) continue;
            perror("accept"); status = EXIT_FAILURE; break;
        }
        deadline = now_ms() + seconds * 1000;
        pid_t pid = fork();
        if (pid == 0) {
            close(listener);
            int result = serve(fd, seconds);
            close(fd);
            _exit(result);
        }
        if (pid < 0) perror("fork");
        else printf("Client served by PID %ld\n", (long)pid);
        close(fd);
    }
    close(listener);
    return status;
}
