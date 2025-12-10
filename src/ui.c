#include <ncurses.h>
#include <unistd.h>   
#include <stdlib.h>  
#include <string.h>   
#include "game_threads.h"
#include "display.h"
#include "board.h"
#include "ui.h"


void screen_refresh(board_t * game_board, int mode) {
    debug("REFRESH\n");
    draw_board(game_board, mode);
    refresh_screen();
    if(game_board->tempo != 0)
    sleep_ms(game_board->tempo);       
}

// Função auxiliar para preparar a memória da cópia local

//####FOI O CHAT NÃO PERCEBO NADA DISTO####
// tinha feito sem isto antes
void setup_snapshot_memory(board_t *snapshot, board_t *original) {
    // 1. Copiar estrutura base (tamanhos, contadores...)
    *snapshot = *original;

    // 2. Alocar memória Própria para os arrays
    // Se não fizéssemos isto, os ponteiros do snapshot apontariam para 
    // a memória real, e teríamos race conditions na mesma!
    snapshot->board = malloc(sizeof(board_pos_t) * original->width * original->height);
    snapshot->pacmans = malloc(sizeof(pacman_t) * original->n_pacmans);
    snapshot->ghosts = malloc(sizeof(ghost_t) * original->n_ghosts);

    if (!snapshot->board || !snapshot->pacmans || !snapshot->ghosts) {
        endwin();
        fprintf(stderr, "Erro Fatal: Falta de memória para o UI Snapshot.\n");
        exit(1);
    }
}

void free_snapshot_memory(board_t *snapshot) {
    free(snapshot->board);
    free(snapshot->pacmans);
    free(snapshot->ghosts);
}

void *ui_thread_func(void *arg) {

    // 1. Recuperar os argumentos
    thread_args_t *args = (thread_args_t*)arg;
    board_t *real_board = args->game_board;
    pthread_mutex_t *mutex = args->board_mutex;
    bool *keep_running = args->keep_running;

    // 2. Criar o Tabuleiro "Snapshot"
    board_t local_board;
    setup_snapshot_memory(&local_board, real_board);

   // int ui_refresh_rate = 30;

    while (*keep_running) {
        
        // dar lock ao mutex antes de aceder ao board real 
        pthread_mutex_lock(mutex);

        // Copiar a grelha (paredes, itens, etc)
        memcpy(local_board.board, real_board->board, 
               sizeof(board_pos_t) * real_board->width * real_board->height);
        
        // Copiar os Pacmans, para saber onde desenhá-lo depois
        memcpy(local_board.pacmans, real_board->pacmans, 
               sizeof(pacman_t) * real_board->n_pacmans);

        // Copiar os Monstros
        memcpy(local_board.ghosts, real_board->ghosts, 
               sizeof(ghost_t) * real_board->n_ghosts);
        
        // /###E preciso copiar mais alguma coisa? Provavelmente não i guess###

        pthread_mutex_unlock(mutex);
    
        // Usamos a cópia local para desenhar o ecrã
        screen_refresh(&local_board, 0);
        /*// Usamos a cópia local para desenhar o ecrã
        draw_board(&local_board, 0); // 0 = Modo Normal
        
        refresh_screen();*/ // Acho que n precisamos de usar estas funções e 
        //apenas usamos o screen_refresh que serve de "atalho" ig
        char ch = get_input();
        if (ch != '\0') {
            pthread_mutex_lock(mutex);

            if (ch == 'Q') {
                real_board->exit_status = QUIT_GAME;

                *keep_running = false;

            } else if (ch == 'G') {
                real_board->exit_status = CREATE_BACKUP;
                *keep_running = false;
            }
            else {
                real_board->next_user_move = ch;
            }
            pthread_mutex_unlock(mutex);
        }
        //##VER SE PODEMOS USAR O USLEEP AQUI## alterar valores conforme necessário
        // Secalhar not sleep pq o scree_refresh ja faz isso
      //  sleep_ms(ui_refresh_rate);
    }

    // Limpar a memória antes de sair
    free_snapshot_memory(&local_board);
    
    return NULL;
}