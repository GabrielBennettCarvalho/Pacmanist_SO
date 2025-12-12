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


void start_threads(
    board_t *board, 
    pthread_mutex_t *board_mutex,    
    bool *keep_running, 
    pthread_t *ui_thread, 
    pthread_t *pacman_thread, 
    pthread_t *monster_threads
);

void stop_threads(
    pthread_t ui_thread, 
    pthread_t pacman_thread, 
    pthread_t *monster_threads, 
    int n_ghosts, 
    bool *keep_running
);



void *monster_thread_func(void *arg);
void *pacman_thread_func(void *arg);
void *ui_thread_func(void *arg);



#endif

