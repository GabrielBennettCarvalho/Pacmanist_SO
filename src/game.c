#include "board.h"
#include "display.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define CONTINUE_PLAY 0
#define NEXT_LEVEL 1
#define QUIT_GAME 2
#define LOAD_BACKUP 3
#define CREATE_BACKUP 4
#define EXIT_RELOAD 42

void screen_refresh(board_t * game_board, int mode) {
    debug("REFRESH\n");
    draw_board(game_board, mode);
    refresh_screen();
    if(game_board->tempo != 0)
        sleep_ms(game_board->tempo);       
}

int play_board(board_t * game_board, bool am_i_child) {
    pacman_t* pacman = &game_board->pacmans[0];
    command_t* play;
    if (pacman->n_moves == 0) { // if is user input
        command_t c; 
        c.command = get_input();

        if(c.command == '\0')
            return CONTINUE_PLAY;

        c.turns = 1;
        play = &c;
    }
    else { // else if the moves are pre-defined in the file
        // avoid buffer overflow wrapping around with modulo of n_moves
        // this ensures that we always access a valid move for the pacman
        play = &pacman->moves[pacman->current_move%pacman->n_moves];
    }

    debug("KEY %c\n", play->command);

    if (play->command == 'Q') {
        return QUIT_GAME;
    }
    if (play->command == 'G') {
        // If I'm the child, I can't save again.
        if (am_i_child) {
            return CONTINUE_PLAY;
        } else

        return CREATE_BACKUP;
    }

    int result = move_pacman(game_board, 0, play);

    if (result == REACHED_PORTAL) {
        // Next level
        return NEXT_LEVEL;
    }

    if(result == DEAD_PACMAN && !am_i_child) {
        return QUIT_GAME;
    }
    if (result == DEAD_PACMAN && am_i_child) {
        return LOAD_BACKUP;
    }
    
    for (int i = 0; i < game_board->n_ghosts; i++) {
        ghost_t* ghost = &game_board->ghosts[i];
        // avoid buffer overflow wrapping around with modulo of n_moves
        // this ensures that we always access a valid move for the ghost
        move_ghost(game_board, i, &ghost->moves[ghost->current_move%ghost->n_moves]);
    }

    if (!game_board->pacmans[0].alive) {
        if (am_i_child) {
            return LOAD_BACKUP;

        } else return QUIT_GAME;
    }      

    return CONTINUE_PLAY;  
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <level_directory>\n", argv[0]);
        // TODO receive inputs
    }

    // Random seed for any random movements
    srand((unsigned int)time(NULL));

    open_debug_file("debug.log");

    terminal_init();
    
    int accumulated_points = 0;
    bool end_game = false;
    board_t game_board;
    int status;
    bool am_i_child = false;

    while (!end_game) {
        load_level(&game_board, accumulated_points);
        draw_board(&game_board, DRAW_MENU);
        refresh_screen();

        while(true) {
            int result = play_board(&game_board, am_i_child); 

            if (result == CREATE_BACKUP) {
                pid_t pid = fork();
                if (pid == -1) {
                    return 0;
                }
                //parent's code
                if (pid > 0) {
                    // Parent freezes and waits for child to die.
                    wait(&status);
                    // Did the child die normally through exit/return instead of a crash?
                    if (WIFEXITED(status)) {
                        // Get the exit code from the child.
                        int exit_code = WEXITSTATUS(status);

                        // Means that the pacman died, so we continue the loop.
                        if (exit_code == LOAD_BACKUP) {
                             screen_refresh(&game_board, DRAW_MENU);
                            continue;
                        // If Child quits, parent quits too.
                        } else if (exit_code == QUIT_GAME) {
                            screen_refresh(&game_board, DRAW_GAME_OVER); 
                            sleep_ms(game_board.tempo);
                            end_game = true;
                            break;
                        }
                        else {
                            end_game = true;
                            break;
                        }
                    }   
                 
                // Child's code
                } if (pid == 0)   {
                    am_i_child = true;

                }
            }

            if (result == LOAD_BACKUP) {
                exit(LOAD_BACKUP);
            }

            if(result == NEXT_LEVEL) {
                screen_refresh(&game_board, DRAW_WIN);
                sleep_ms(game_board.tempo);
                break;
            }


            
            if(result == QUIT_GAME) {
                screen_refresh(&game_board, DRAW_GAME_OVER); 
                sleep_ms(game_board.tempo);

                if (am_i_child) {
                    exit(QUIT_GAME);
                }

                end_game = true;
                break;
            }
        
    
            screen_refresh(&game_board, DRAW_MENU); 

            accumulated_points = game_board.pacmans[0].points;      
        }
        print_board(&game_board);
        unload_level(&game_board);
    }    

    terminal_cleanup();

    close_debug_file();

    return 0;
}
