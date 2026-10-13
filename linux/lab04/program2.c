#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct write_args {
    FILE *file;
    const char *line;
    int error;
};
static void *write_line(void *arg)
{
    struct write_args *args = arg;
    if (fputs(args->line, args->file) == EOF || fflush(args->file) == EOF) {
        args->error = 1;
    }
    return NULL;
}
static int start_thread(pthread_t *thread, struct write_args *args)
{
    int error = pthread_create(thread, NULL, write_line, args);
    if (error != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(error));
        return -1;
    }
    return 0;
}
static int wait_thread(pthread_t thread)
{
    int error = pthread_join(thread, NULL);
    if (error != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(error));
        return -1;
    }
    return 0;
}
int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr,
                "Использование: %s ВХОДНОЙ_ФАЙЛ НЕЧЁТНЫЕ_СТРОКИ "
                "ЧЁТНЫЕ_СТРОКИ\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    FILE *input = fopen(argv[1], "r");
    FILE *odd = fopen(argv[2], "w");
    FILE *even = fopen(argv[3], "w");
    if (input == NULL || odd == NULL || even == NULL) {
        perror("fopen");
        if (input != NULL) fclose(input);
        if (odd != NULL) fclose(odd);
        if (even != NULL) fclose(even);
        return EXIT_FAILURE;
    }
    char *odd_line = NULL;
    char *even_line = NULL;
    size_t odd_capacity = 0;
    size_t even_capacity = 0;
    int result = EXIT_SUCCESS;
    while (getline(&odd_line, &odd_capacity, input) != -1) {
        ssize_t even_length = getline(&even_line, &even_capacity, input);
        struct write_args odd_args = { odd, odd_line, 0 };
        struct write_args even_args = { even, even_line, 0 };
        pthread_t odd_thread;
        pthread_t even_thread;
        if (start_thread(&odd_thread, &odd_args) == -1) {
            result = EXIT_FAILURE;
            break;
        }
        int even_started = 0;
        if (even_length != -1) {
            if (start_thread(&even_thread, &even_args) == -1) {
                wait_thread(odd_thread);
                result = EXIT_FAILURE;
                break;
            }
            even_started = 1;
        }
        if (wait_thread(odd_thread) == -1) {
            result = EXIT_FAILURE;
        }
        if (even_started && wait_thread(even_thread) == -1) {
            result = EXIT_FAILURE;
        }
        if (odd_args.error || (even_started && even_args.error)) {
            fprintf(stderr, "Ошибка записи в выходной файл.\n");
            result = EXIT_FAILURE;
        }
        if (result != EXIT_SUCCESS) {
            break;
        }
    }
    if (ferror(input)) {
        perror("getline");
        result = EXIT_FAILURE;
    }
    free(odd_line);
    free(even_line);
    if (fclose(input) == EOF) {
        perror("fclose input");
        result = EXIT_FAILURE;
    }
    if (fclose(odd) == EOF) {
        perror("fclose odd");
        result = EXIT_FAILURE;
    }
    if (fclose(even) == EOF) {
        perror("fclose even");
        result = EXIT_FAILURE;
    }
    return result;
}
