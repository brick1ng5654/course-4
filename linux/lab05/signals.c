#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void handle_fault(int signo)
{
    if (signo == SIGFPE) {
        const char msg[] = "Ошибка деления на ноль: SIGFPE\n";
        (void)write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(1);
    }
    if (signo == SIGSEGV) {
        const char msg[] = "Ошибка обращения к памяти: SIGSEGV\n";
        (void)write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(2);
    }
    _exit(3);
}
int main(int argc, char *argv[])
{
    if (argc != 2 ||
        (strcmp(argv[1], "div0") != 0 &&
         strcmp(argv[1], "ptr") != 0)) {
        fprintf(stderr, "Использование: %s div0|ptr\n", argv[0]);
        return EXIT_FAILURE;
    }
    struct sigaction action = {0};
    action.sa_handler = handle_fault;
    if (sigemptyset(&action.sa_mask) == -1 ||
        sigaction(SIGFPE, &action, NULL) == -1 ||
        sigaction(SIGSEGV, &action, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "div0") == 0) {
        puts("Проверка деления на ноль");
        fflush(stdout);
        volatile int dividend = 10, zero = 0;
        volatile int result = dividend / zero;
        (void)result;
    } else {
        puts("Проверка недопустимого указателя");
        fflush(stdout);
        volatile int *ptr = NULL;
        *ptr = 42;
    }
    return EXIT_SUCCESS;
}
