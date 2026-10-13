#define _GNU_SOURCE

#include "process_info.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Use %s FILE DELAY\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned delay = parse_delay(argv[2]);
    // this programm execute as second child after execl
    write_attributes("Child 2 (vfork + exec)", argv[1], delay);

    return EXIT_SUCCESS;
}
