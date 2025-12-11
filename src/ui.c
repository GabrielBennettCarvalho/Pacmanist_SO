#include <ncurses.h>
#include <unistd.h>   
#include <stdlib.h>  
#include <string.h>   
#include "game_threads.h"
#include "display.h"
#include "board.h"
#include "ui.h"


void screen_refresh(board_t * game_board, int mode) {
   // debug("REFRESH\n");
    draw_board(game_board, mode);
    refresh_screen();
    if(game_board->tempo != 0)
    sleep_ms(game_board->tempo);       
}

// Helper function to prepare the local snapshot memory
// This implements a "Deep Copy" strategy.
void setup_snapshot_memory(board_t *snapshot, board_t *original) {
    // 1. Copy base structure (scalar values like width, height, counters...)
    *snapshot = *original;

    // 2. Allocate OWN memory for the arrays.
    // Explanation: If we didn't do this, 'snapshot->board' would point to the exact same 
    // memory address as 'original->board'. This would cause race conditions because 
    // the UI would be reading while the game logic writes to the same location.
    // By malloc-ing here, we create a separate buffer just for the UI.
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

    // Extract arguments
    thread_args_t *args = (thread_args_t*)arg;
    board_t *real_board = args->game_board;
    pthread_mutex_t *mutex = args->board_mutex;
    bool *keep_running = args->keep_running;

    // Create the "Snapshot" Board
    // This local_board will be used exclusively for drawing
    board_t local_board;
    setup_snapshot_memory(&local_board, real_board);

   // int ui_refresh_rate = 30;

    while (*keep_running) {
        
        // Lock mutex only to copy the data state 
        pthread_mutex_lock(mutex);

        // Copiar a grelha (paredes, itens, etc)
        memcpy(local_board.board, real_board->board, 
               sizeof(board_pos_t) * real_board->width * real_board->height);
        
        // Copy the grid content (walls, items, etc)
        memcpy(local_board.pacmans, real_board->pacmans, 
               sizeof(pacman_t) * real_board->n_pacmans);

        // Copy ghosts
        memcpy(local_board.ghosts, real_board->ghosts, 
               sizeof(ghost_t) * real_board->n_ghosts);
        

        pthread_mutex_unlock(mutex);
    
        // We use the local copy to draw the screen. 
        // This allows the Game Logic thread to proceed with calculations immediately 
        // without waiting for the slow I/O of drawing to the terminal.
        screen_refresh(&local_board, DRAW_MENU);

        
        refresh_screen(); // We likely don't need these raw functions as screen_refresh wraps them ig

        // Handle Input
        // Input needs to write to the REAL board, so we need the lock again for a short moment.
        char ch = get_input();
        if (ch != '\0') {
            pthread_mutex_lock(mutex);

            if (ch == 'Q') {
                real_board->exit_status = QUIT_GAME;

                *keep_running = false;

            } else if (ch == 'G') {
                if (real_board->can_save) {
                real_board->exit_status = CREATE_BACKUP;
                *keep_running = false;
                }
            }
            else {
                // Pass movement command to the logic thread
                real_board->next_user_move = ch;
            }
            pthread_mutex_unlock(mutex);
        }
    }

    // Clean up memory before exiting to avoid leaks
    free_snapshot_memory(&local_board);
    
    return NULL;
}