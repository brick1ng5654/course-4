#include "rw.h"

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--init") == 0) {
        FILE *file = fopen(argv[2], "a");
        if (!file) die("fopen");
        if (fclose(file) == EOF) die("fclose");
        key_t key = ftok(argv[2], 'W');
        if (key == (key_t)-1) die("ftok");
        int id = semget(key, 3, IPC_CREAT | IPC_EXCL | 0600);
        if (id == -1) die("semget --init");
        unsigned short values[] = {0, 0, 0};
        union semun arg = {.array = values};
        if (semctl(id, 0, SETALL, arg) == -1) {
            int saved = errno;
            semctl(id, 0, IPC_RMID);
            errno = saved;
            die("SETALL");
        }
        printf("Initialized: semid=%d\n", id);
        return EXIT_SUCCESS;
    }
    if (argc == 3 && strcmp(argv[1], "--cleanup") == 0) {
        int id = file_semaphores(argv[2]);
        if (semctl(id, 0, IPC_RMID) == -1) die("IPC_RMID");
        puts("Semaphores removed.");
        return EXIT_SUCCESS;
    }
    if (argc < 4 || argc > 6) {
        fprintf(stderr, "Usage: %s FILE TEXT N [DELAY_MS [HOLD_MS]]\n"
                "       %s --init|--cleanup FILE\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }
    int n = number(argv[3]);
    int delay = argc >= 5 ? number(argv[4]) : 0;
    int hold = argc == 6 ? number(argv[5]) : 0;
    int id = file_semaphores(argv[1]);
    setbuf(stdout, NULL);
    for (int i = 0; i < n; ++i) {
        pause_ms(delay);
        printf("%s pid=%ld: REQUEST %d\n", argv[0], (long)getpid(), i+1);
        writer_enter(id);
        printf("%s pid=%ld: WRITE BEGIN\n", argv[0], (long)getpid());
        FILE *file = fopen(argv[1], "a");
        if (!file) die("fopen");
        if (fprintf(file, "%s\n", argv[2]) < 0) die("fprintf");
        pause_ms(hold);
        if (fclose(file) == EOF) die("fclose");
        printf("%s pid=%ld: WRITE END: %s\n",
               argv[0], (long)getpid(), argv[2]);
        writer_leave(id);
    }
    return EXIT_SUCCESS;
}
