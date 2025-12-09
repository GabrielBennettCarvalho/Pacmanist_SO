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
    pthread_t *monster_threads, //guardar os ids das threads monstros
    thread_args_t *args_storage  //mudar para moster_args talvez    
) {

    // Initialize mutex do board que só vai ser destruido no close_threads
    pthread_mutex_init(board_mutex, NULL);

    *keep_running = true;

    //para não acabarem quando a função acabar
    static thread_args_t ui_args;
    static thread_args_t pacman_args;

    // Configurar argumentos da thread UI
    ui_args.game_board = board;
    ui_args.board_mutex = board_mutex;
    ui_args.keep_running = keep_running;
    ui_args.entity_id = -1; // UI thread id

    pthread_create(ui_thread, NULL, ui_thread_func, &ui_args);

    //configurar o pacman mais ou menos igual

    //lançar thread do pacman


    for (int i = 0; i < board->n_ghosts; i++) {

        args_storage[i].game_board = board;
        args_storage[i].board_mutex = board_mutex;
        args_storage[i].keep_running = keep_running;
        
        args_storage[i].entity_id = i;

        // Create monster thread ####talvez mudar o nome para moster_threads[i]#####
        pthread_create(&monster_threads[i], NULL, monster_thread_func, &args_storage[i]);

    }


}

void stop_threads(
    pthread_t ui_thread, 
    pthread_t pacman_thread, 
    pthread_t *monster_threads, 
    int n_ghosts,
    bool *keep_running,          // Precisamos do ponteiro para a flag
    pthread_mutex_t *board_mutex
) {

    *keep_running = false;

    // Esperar pela Thread do pacman
    pthread_join(pacman_thread, NULL);

    // Esperar pelas threads dos Monstros
    for (int i = 0; i < n_ghosts; i++) {
        pthread_join(monster_threads[i], NULL);
    }

    // Esperar pela Thread da UI
    pthread_join(ui_thread, NULL);

    // Destruir mutex
    pthread_mutex_destroy(board_mutex);    
}

void *monster_thread_func(void *arg) {
    // cast
    thread_args_t *args = (thread_args_t*)arg;

    board_t *board = args->game_board;
    pthread_mutex_t *mutex = args->board_mutex;
    int id = args->entity_id;
}


