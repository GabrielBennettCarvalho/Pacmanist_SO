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
    bool *keep_running = args->keep_running;

    ghost_t *ghost = &board->ghosts[id];
    // While the game is running
    // Pointer so it updates "in real time" and it ain't a copy.
    while (*keep_running) {

        // Critical Section
        pthread_mutex_lock(mutex);

        // In case the user quits while this thread is inside the lock
        if (!*keep_running) {
            pthread_mutex_unlock(mutex);
            break;
        }

        command_t *next_move = &ghost->moves[ghost->current_move%ghost->n_moves];

        int result = move_ghost(board, id, next_move);

        if (result == DEAD_PACMAN) {
            *keep_running = false;
        }

        pthread_mutex_unlock(mutex);

        if (*keep_running == false) break;

        sleep_ms(board->tempo);
    }
    return NULL;
}



void *pacman_thread_func(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;

    board_t *board = args->game_board;
    pthread_mutex_t *mutex = args->board_mutex;
    int id = args->entity_id; // maybe not needed
    bool *keep_running = args->keep_running;

    pacman_t *pacman = &board->pacmans[0];
    
    command_t c; 
    while(*keep_running) {

        pthread_mutex_lock(mutex);

        // Second check to impede race conditions
        if (!*keep_running || !pacman->alive) {
        pthread_mutex_unlock(mutex);
        break; // leave loop
        }


        command_t *play;
        if (pacman->n_moves == 0) { // if is user input
        c.command = board->next_user_move;
        // Clean input
        board->next_user_move = '\0';
        if(c.command == '\0') {
            pthread_mutex_unlock(mutex);
            sleep_ms(10);
            continue;
        }

        c.turns = 1;
        play = &c;
        }
        else { 
        play = &pacman->moves[pacman->current_move%pacman->n_moves];
        }

        int result = move_pacman(board, 0, play);

        pthread_mutex_unlock(mutex);

        sleep_ms(board->tempo);
    }
    return NULL;
}