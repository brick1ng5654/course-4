#include "common.h"

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input odd_output even_output\n", argv[0]);
        return EXIT_FAILURE;
    }
    FILE *input = fopen(argv[1], "r");
    if (input == NULL) fail("fopen input");
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGUSR1);
    sigaddset(&signals, SIGUSR2);
    sigaddset(&signals, SIGQUIT);
    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1) fail("sigprocmask");
    int channel[2];
    if (pipe(channel) == -1) fail("pipe");
    pid_t children[2];
    for (int i = 0; i < 2; ++i) {
        children[i] = fork();
        if (children[i] == -1) fail("fork");
        if (children[i] == 0) {
            close(channel[1]);
            fclose(input);
            char descriptor[32], number[8], peer[32];
            snprintf(descriptor, sizeof descriptor, "%d", channel[0]);
            snprintf(number, sizeof number, "%d", i + 1);
            snprintf(peer, sizeof peer, "%ld",
                     i == 0 ? 0L : (long)children[0]);
            execl("./reader", "reader", descriptor, number, peer,
                  argv[i + 2], (char *)NULL);
            perror("execl reader");
            _exit(EXIT_FAILURE);
        }
    }
    close(channel[0]);
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    while ((length = getline(&line, &capacity, input)) != -1)
        write_all(channel[1], line, (size_t)length);
    if (ferror(input)) fail("getline");
    free(line);
    fclose(input);
    if (kill(children[0], SIGQUIT) == -1) fail("kill SIGQUIT");
    if (kill(children[1], SIGQUIT) == -1) fail("kill SIGQUIT");
    close(channel[1]);
    int first_ok = wait_child(children[0]);
    int second_ok = wait_child(children[1]);
    return first_ok && second_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
