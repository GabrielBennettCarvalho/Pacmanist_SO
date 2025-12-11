#include "board.h"
#include "display.h"
#include "loader.h"
#include "game_threads.h"
#include "ui.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>




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

    char path[256];

    if (argc != 2) {
        printf("Usage: %s <level_directory>\n", argv[0]);
        scanf("%255s", path);
    } else {
        // Se argc == 2, TEMOS de copiar o argv[1] para a variável path
        strcpy(path, argv[1]); 
    }


    LevelList *levels = load_directory(path);

    
    if (levels == NULL) {
        fprintf(stderr, "Error: load_directory returned NULL.\n");
        return 1;
    }

    if(levels->size == 0) {
        free_level_list(levels);
        return 1;
    }

    // Random seed for any random movements
    srand((unsigned int)time(NULL));

    open_debug_file("debug.log");

    terminal_init();
    
    int current_lvl_idx = 0;
    int accumulated_points = 0;
    bool end_game = false;
    board_t game_board;
    int status;
    int result;
    bool am_i_child = false;

    bool reload_level = true;

    pthread_t ui_thread;
    pthread_t pacman_thread;
    pthread_t monster_threads[MAX_GHOSTS];
    
    pthread_mutex_t board_mutex;

    // initialization of mutex
    if (pthread_mutex_init(&board_mutex, NULL) != 0 ) {
        return 1;
    }

    while (current_lvl_idx < levels->size && !end_game) {
    
        
        // Use the flag to know if we should go to next level or simply go back to where we were at.
        if (reload_level) {
            char level_full_path[512];
            snprintf(level_full_path, sizeof(level_full_path), "%s/%s", path, levels->level_names[current_lvl_idx]);
            
    
            if (load_level_from_file(&game_board, level_full_path, accumulated_points, path) != 0) {
                fprintf(stderr, "Error reading level %s\n", level_full_path);
                break;
            }
            sprintf(game_board.level_name, "LEVEL %d", current_lvl_idx + 1);
        }

        // Initialize game
        game_board.exit_status = CONTINUE_PLAY;
        game_board.can_save = !am_i_child;
        bool threads_running = true;

        start_threads(&game_board, &board_mutex, &threads_running, &ui_thread, &pacman_thread, monster_threads);

        while (threads_running) {

            // Check if the status is still CONTINUE_PLAY
            pthread_mutex_lock(&board_mutex);
            result = game_board.exit_status;
            pthread_mutex_unlock(&board_mutex);

            if (result != CONTINUE_PLAY) {
                threads_running = false;
            } else {
                // We put the CPU asleep so it isn't always with the mutex in his hand
                sleep_ms(10);
            }
        }

        stop_threads(ui_thread,pacman_thread,monster_threads, game_board.n_ghosts, &threads_running);
        result = game_board.exit_status;

        if (result == CREATE_BACKUP) {
            pid_t pid = fork();
            if (pid < 0) {
                end_game = true; // Fatal error
            } 
            else if (pid > 0) {
                wait(&status);

                 // Did the child die normally through exit/return instead of a crash?
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);

                // Means that the pacman died with a backup, so we continue the loop and the threads start again.
                    if (exit_code == LOAD_BACKUP) {
                        reload_level = false;
                        continue;
                    }
                    else if (exit_code == QUIT_GAME) {
                        // Here we need screen_refresh because the ui thread won't restart
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
            else if (WIFSIGNALED(status)) {
                     // If child crashed, we treat it as a  game over
                     // Here we end game to stop infinite loops.
                     end_game = true;
                     break;
                }
        }
            if (pid == 0)   {
                    am_i_child = true;
                    reload_level = false;
                    continue;
                }
        }
        

        if (result == PACMAN_DIED) {
            if (am_i_child) exit(LOAD_BACKUP);
            else {
                screen_refresh(&game_board, DRAW_GAME_OVER); 
                sleep_ms(2000);
                end_game = true;
                break;
            }
        }
        if (result == NEXT_LEVEL) {
            screen_refresh(&game_board, DRAW_WIN);
            sleep_ms(700);
            accumulated_points = game_board.pacmans[0].points;

            unload_level(&game_board);
            current_lvl_idx++;
            reload_level = true;    
            continue;
        } 
        // If the user pressed Q
        if (result == QUIT_GAME) {
            screen_refresh(&game_board, DRAW_GAME_OVER); 
            sleep_ms(2000);

            if (am_i_child) {
                    exit(QUIT_GAME);
            }

            end_game = true;
            break;
        }
        // If loop ends without continue, cleanup
        //if (reload_level) unload_level(&game_board);

        print_board(&game_board);
    }    
    
    terminal_cleanup();
    unload_level(&game_board);

    free_level_list(levels);

    pthread_mutex_destroy(&board_mutex);

    close_debug_file();
    

    return 0;
}   
