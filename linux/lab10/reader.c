#include "rw.h"

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage: %s FILE [DELAY_MS [HOLD_MS]]\n", argv[0]);
        return EXIT_FAILURE;
    }
    int delay = argc >= 3 ? number(argv[2]) : 0;
    int hold = argc == 4 ? number(argv[3]) : 0;
    int id = file_semaphores(argv[1]);
    setbuf(stdout, NULL);
    pause_ms(delay);
    fprintf(stderr, "%s pid=%ld: REQUEST\n", argv[0], (long)getpid());
    reader_enter(id);
    fprintf(stderr, "%s pid=%ld: READ BEGIN\n", argv[0], (long)getpid());
    FILE *file = fopen(argv[1], "r");
    if (!file) die("fopen");
    int c;
    while ((c = fgetc(file)) != EOF)
        if (putchar(c) == EOF) die("stdout");
    if (ferror(file)) die("fgetc");
    if (fclose(file) == EOF) die("fclose");
    pause_ms(hold);
    fprintf(stderr, "%s pid=%ld: READ END\n", argv[0], (long)getpid());
    reader_leave(id);
    return EXIT_SUCCESS;
}
