#define _GNU_SOURCE

#include "process_info.h"
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    // name of file and three delay values
    if (argc != 5) {
        fprintf(stderr,
                "Use: %s FILE PARENT_DELAY "
                "CHILD1_DELAY CHILD2_DELAY\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *output_file = argv[1];
    unsigned parent_delay = parse_delay(argv[2]);
    unsigned child1_delay = parse_delay(argv[3]);
    unsigned child2_delay = parse_delay(argv[4]);

    // create new file and write launch parameters
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    dprintf(fd,
            "parameters:\n"
            "output file = %s\n"
            "parent delay = %u c\n"
            "child 1 delay = %u c\n"
            "child 2 delay = %u c\n\n",
            output_file,
            parent_delay,
            child1_delay,
            child2_delay);
    close(fd);

    // first child continue execution current programm
    pid_t child1 = fork();
    if (child1 == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child1 == 0) {
        write_attributes("Child 1 (fork)", output_file, child1_delay);
        _exit(EXIT_SUCCESS);
    }

    // second child immediately change program via execl
    pid_t child2 = vfork();
    if (child2 == -1) {
        perror("vfork");
        return EXIT_FAILURE;
    }
    if (child2 == 0) {
        execl("./child2", "child2", output_file, argv[4], (char *)NULL);
        _exit(127);
    }

    write_attributes("Parent", output_file, parent_delay);

    // parent wait ending of both childs
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    return EXIT_SUCCESS;
}
