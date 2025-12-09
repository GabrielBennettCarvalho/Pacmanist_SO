#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "game_threads.h"
#include "board.h"

void start_threads(board_t *board,
                   pthread_mutex_t *board_mutex,
                   bool *keep_running,
                   pthread_t *thread_ids,
                   thread_args_t *args_storage
) {

    for (int i = 0; i < board->n_ghosts; i++) {

        args_storage[i].game_board = board;
        args_storage[i].board_mutex = board_mutex;
        args_storage[i].keep_running = keep_running;
        
        args_storage[i].entity_id = i;

        pthread_create(&thread_ids[i], NULL, monster_thread_func, &args_storage[i]);

    }


}

void *monster_thread_func(void *arg) {
    // cast
    thread_args_t *args = (thread_args_t*)arg;

    board_t *board = args->game_board;
    pthread_mutex_t *mutex = args->board_mutex;
    int id = args->entity_id;
}


