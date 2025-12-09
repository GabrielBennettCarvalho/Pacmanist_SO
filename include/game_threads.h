#ifndef GAME_THREADS_H
#define GAME_THREADS_H

#include <pthread.h>
#include <stdbool.h>
#include "board.h"

typedef struct {
    board_t *game_board;
    pthread_mutex_t *board_mutex;
    bool *keep_running;
    int entity_id;          // Id of monsters (1,2,3...) or -1 if pacman
} thread_args_t;


void start_threads(board_t *board,
    pthread_mutex_t *board_mutex,
    bool *keep_running,
    pthread_t *thread_ids,
    thread_args_t *args_storage
);

void *monster_thread_func(void *arg);
void *pacman_thread_func(void *arg);
void *ui_thread_func(void *arg);



#endif

