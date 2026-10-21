#ifndef RW_H
#define RW_H
#include "ipc.h"

enum { WAIT_WRITERS, ACTIVE_WRITER, ACTIVE_READERS };

static inline int file_semaphores(const char *path)
{
    key_t key = ftok(path, 'W');
    if (key == (key_t)-1) die("ftok");
    int id = semget(key, 3, 0600);
    if (id == -1) die("semget: run writer --init first");
    return id;
}

static inline void writer_enter(int id)
{
    struct sembuf register_writer = {WAIT_WRITERS, 1, SEM_UNDO};
    operations(id, &register_writer, 1);
    struct sembuf enter[] = {
        {ACTIVE_READERS, 0, 0},
        {ACTIVE_WRITER, 0, 0},
        {WAIT_WRITERS, -1, SEM_UNDO},
        {ACTIVE_WRITER, 1, SEM_UNDO}
    };
    operations(id, enter, 4);
}

static inline void writer_leave(int id)
{
    struct sembuf leave = {ACTIVE_WRITER, -1, SEM_UNDO};
    operations(id, &leave, 1);
}

static inline void reader_enter(int id)
{
    struct sembuf enter[] = {
        {WAIT_WRITERS, 0, 0},
        {ACTIVE_WRITER, 0, 0},
        {ACTIVE_READERS, 1, SEM_UNDO}
    };
    operations(id, enter, 3);
}

static inline void reader_leave(int id)
{
    struct sembuf leave = {ACTIVE_READERS, -1, SEM_UNDO};
    operations(id, &leave, 1);
}
#endif
