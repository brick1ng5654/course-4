#include "common.h"

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s server_ip timeout_seconds\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    int seconds = timeout_arg(argv[2]);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, argv[1], &address.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address\n");
        return EXIT_FAILURE;
    }
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) { perror("socket"); return EXIT_FAILURE; }
    if (connect(fd, (struct sockaddr *)&address,
                sizeof(address)) < 0) {
        if (errno != EINPROGRESS) goto error;
        if (wait_ready(fd, POLLOUT,
                       now_ms() + seconds * 1000) < 0) goto error;
        int socket_error = 0;
        socklen_t length = sizeof(socket_error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR,
                       &socket_error, &length) < 0) goto error;
        if (socket_error) { errno = socket_error; goto error; }
    }
    puts("Connected to server");
    uint32_t numbers[COUNT];
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    for (int i = 0; i < COUNT; ++i)
        numbers[i] = (uint32_t)(rand() % 100);
    print_numbers("generated", numbers);
    for (int i = 0; i < COUNT; ++i)
        numbers[i] = htonl(numbers[i]);
    if (transfer(fd, numbers, sizeof(numbers), 1, seconds) < 0)
        goto error;
    if (transfer(fd, numbers, sizeof(numbers), 0, seconds) < 0)
        goto error;
    for (int i = 0; i < COUNT; ++i)
        numbers[i] = ntohl(numbers[i]);
    print_numbers("received", numbers);
    close(fd);
    return EXIT_SUCCESS;
error:
    perror("client: connect/send/receive");
    close(fd);
    return EXIT_FAILURE;
}
