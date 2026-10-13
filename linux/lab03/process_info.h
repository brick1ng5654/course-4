#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

// transform string argument at delay value
static unsigned parse_delay(const char *value){
    char *end = NULL;
    unsigned long delay = strtoul(value, &end, 10);

    if (*value == '\0' || *end != '\0' || delay > 3600) {
        fprintf(stderr, "Incorrect delay: %s\n", value);
        exit(EXIT_FAILURE);
    }

    return (unsigned)delay;
}

// process delay and write its attributes in shared file
static void write_attributes(const char *process_name,
const char *output_file, unsigned delay){
    sleep(delay);

    int fd = open(output_file, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("open");
        _exit(EXIT_FAILURE);
    }

    dprintf(fd,
            "%s:\n"
            "delay = %u c\n"
            "PID = %ld\n"
            "PPID = %ld\n"
            "SID = %ld\n"
            "PGID = %ld\n"
            "UID = %ld\n"
            "EUID = %ld\n"
            "GID = %ld\n"
            "EGID = %ld\n\n",
            process_name,
            delay,
            (long)getpid(),
            (long)getppid(),
            (long)getsid(0),
            (long)getpgid(0),
            (long)getuid(),
            (long)geteuid(),
            (long)getgid(),
            (long)getegid());

    close(fd);
}

#endif
