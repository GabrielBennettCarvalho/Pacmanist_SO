#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "game_threads.h"
#include "board.h"


void start_threads(
    board_t *board,
    pthread_mutex_t *board_mutex,
    bool *keep_running,
    pthread_t *ui_thread, //guardar o id da thread ui
    pthread_t *pacman_thread, //guardar o id da thread pacman
    pthread_t *monster_threads //guardar os ids das threads monstros     
) {

    *keep_running = true;

    //para não acabarem quando a função acabar
    static thread_args_t ui_args;
    static thread_args_t pacman_args;
    static thread_args_t monster_args[MAX_GHOSTS];


    pacman_args.game_board = board;
    pacman_args.board_mutex = board_mutex;
    pacman_args.keep_running = keep_running;
    //pacman_args.entity_id = 0;

    // Configurar argumentos da thread UI
    ui_args.game_board = board;
    ui_args.board_mutex = board_mutex;
    ui_args.keep_running = keep_running;
    //ui_args.entity_id = -1; // UI thread id

    pthread_create(ui_thread, NULL, ui_thread_func, &ui_args);

    //lançar thread do pacman
    pthread_create(pacman_thread, NULL, pacman_thread_func, &pacman_args);

    for (int i = 0; i < board->n_ghosts; i++) {

        monster_args[i].game_board = board;
        monster_args[i].board_mutex = board_mutex;
        monster_args[i].keep_running = keep_running;
        
        monster_args[i].entity_id = i;

        // Create monster thread ####talvez mudar o nome para moster_threads[i]#####
        pthread_create(&monster_threads[i], NULL, monster_thread_func, &monster_args[i]);

    }


}

void stop_threads(
    pthread_t ui_thread, 
    pthread_t pacman_thread, 
    pthread_t *monster_threads, 
    int n_ghosts,
    bool *keep_running
    //pthread_mutex_t *board_mutex
) {

    // Might be redundant, but this way we guarantee the other threads stop 
    // in case main didn't change this flag
    *keep_running = false;

    // Esperar pela Thread do pacman
    pthread_join(pacman_thread, NULL);

    // Esperar pelas threads dos Monstros
    for (int i = 0; i < n_ghosts; i++) {
        pthread_join(monster_threads[i], NULL);
    }

    // Esperar pela Thread da UI
    pthread_join(ui_thread, NULL);

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
            board->exit_status = PACMAN_DIED;
            pthread_mutex_unlock(mutex);
            break;
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
        c.command = board->next_user_move;
        if (pacman->n_moves == 0) { // if is user input
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

            if (play->command == 'Q') {
                *keep_running = false;
                board->exit_status = QUIT_GAME;
                pthread_mutex_unlock(mutex);

                break;
            }
            if (play->command == 'G') {
                // Check if we can save
                if (board->can_save) {
                    board->exit_status = CREATE_BACKUP;
                    *keep_running = false; // Stop threads only if valid
                    pacman->current_move++;
                    pthread_mutex_unlock(mutex);
                    break;
                } 
                else {
                    // WE ARE ALREADY A CHILD (BACKUP)
                    pacman->current_move++; // Consume the 'G' so we don't get stuck
                    
                    pthread_mutex_unlock(mutex);
                    sleep_ms(board->tempo); 
                    continue; 
                }
            }
        }

        int result = move_pacman(board, 0, play);


        // in case the pacman kills himself
        if (result == DEAD_PACMAN) {
            *keep_running = false;
            board->exit_status = PACMAN_DIED; 
            pthread_mutex_unlock(mutex);    
            break;
        }
        if (result == REACHED_PORTAL) {
            //keep_running = false;
            board->exit_status = NEXT_LEVEL;
            pthread_mutex_unlock(mutex);
            break;
        }

        pthread_mutex_unlock(mutex);

        if (*keep_running == false) break;

        sleep_ms(board->tempo);
    }
    return NULL;
}