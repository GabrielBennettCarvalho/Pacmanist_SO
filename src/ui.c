#include <ncurses.h>
#include <unistd.h>   
#include <stdlib.h>  
#include <string.h>   
#include "game_threads.h"
#include "display.h"
#include "board.h"
#include "ui.h"

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
        draw_board(&local_board, 0); // 0 = Modo Normal
        
        refresh_screen();

        char ch = get_input();
        if (ch != '\0') {
            pthread_mutex_lock(mutex);

            if (ch == 'Q') {
                *keep_running = false;int main(int argc, char** argv) {

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
    bool am_i_child = false;


    // Thread arguments initialization
    pthread_t thread_ids[MAX_GHOSTS + 1];
    thread_args_t thread_args[MAX_GHOSTS + 1];

    pthread_mutex_t board_mutex;
    bool game_running = false;

    // initialization of mutex
    if (pthread_mutex_init(&board_mutex, NULL) != 0 ) {
        return 1;
    }

    while (current_lvl_idx < levels->size && !end_game) {
    
        
        char level_full_path[512];

        // Build the full path
        snprintf(level_full_path, sizeof(level_full_path), "%s/%s", path, levels->level_names[current_lvl_idx]);
        // Write the number of the level on screen
        sprintf(game_board.level_name, "LEVEL %d", current_lvl_idx + 1);

        if (load_level_from_file(&game_board, level_full_path, accumulated_points, path) != 0) {
            //error
            fprintf(stderr, "Error reading level %s\n", level_full_path);
            break;
        }


        // Initialize threads... TODO

        draw_board(&game_board, DRAW_MENU);
        refresh_screen();
        // PRobably change logic of this!
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
                current_lvl_idx++;
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

    free_level_list(levels);

    close_debug_file();
    

    return 0;
}


            } else if (ch == 'G') {
                // TODO with saving/fork logic or not
            }
            else {
                real_board->next_user_move = ch;
            }
            pthread_mutex_unlock(mutex);
        }
        //##VER SE PODEMOS USAR O USLEEP AQUI## alterar valores conforme necessário
        sleep_ms(real_board->tempo);
    }

    // Limpar a memória antes de sair
    free_snapshot_memory(&local_board);
    
    return NULL;
}