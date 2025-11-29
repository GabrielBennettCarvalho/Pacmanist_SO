#include <board.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


char *read_into_buffer(int fd) {
    // Get all of the file's statistics (to find its size)
    struct stat file_stat; 
    if (fstat(fd, &file_stat) == -1) {
       return NULL;
    }
    // Get file's size
    off_t file_size = file_stat.st_size;

    // Alocate size (in heap) for the buffer to avoid stack overflows
    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        return NULL;
    }

    // Read the file
    ssize_t bytes_read = read(fd, buffer, file_size);
    if (bytes_read != file_size) {
        // If it's not the same size, something went wrong reading.
        free(buffer);
        return NULL;
    }

    // Null terminate the buffer to treat it like a string.
    buffer[file_size] = '\0';

    return buffer;
}

int load_level_from_file(board_t *board, const char *level_path, int accumulated_points) {

    //Open file
    int fd = open(level_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    char *buffer = read_into_buffer(fd);

    // Close the file as we don't need it anymore.
    close(fd);

    if (buffer == NULL) {
        return 1;
    }


    char *line = strtok(buffer, "\n");
    /* GUSTAVO: For you my g
    O strtok divide o buffer em tokens, neste caso até encontrar o \n,
    quando o encontrar, devolve tudo para tras dele e fica com um ponteiro '\0'
    a apontar para onde parou, por isso é que quando fazemos strtok(NULL, ...) 
    ele lê onde "estava".
    */

    int w, h, tempo;
    int map_row = 0;

    // Safety initializing
    board->n_ghosts = 0;
    board->board = NULL;

    while (line != NULL) {        
        if (line[0] == '#' || line[0] == '\n') {
            // Do nothing, let it reach the bottom to get to the next line
        }
        // Read Dimensions
        else if (strncmp(line, "DIM", 3) == 0) {
            if (sscanf(line + 3, "%d %d", &w, &h) == 2) { 
                board->width = w;
                board->height = h;

                board->board = calloc(board->width * board->height, sizeof(board_pos_t));
            }
        }
        // Read Tempo
        else if (strncmp(line, "TEMPO", 5) == 0) {
            if (sscanf(line + 5, "%d", &tempo) == 1) {
                board->tempo = tempo;
            }
        }
        // Read Pacman file
        else if (strncmp(line, "PAC", 3) == 0) {
           sscanf(line + 3, "%255s", board->pacman_file);
        }
        else if (strncmp(line, "MON", 3) == 0) {
           char *ptr = line + 3; // Skip MON
           int offset;

           while (board->n_ghosts < MAX_GHOSTS &&
                sscanf(ptr, "%255s%n", board->ghosts_files[board->n_ghosts], &offset) == 1) {
                ptr += offset;
                board->n_ghosts++;
           }

        } else {
            // If it's not a keyword, it's the board
            // Safety check: Don't write to board if DIM wasn't read yet
            if (board->board != NULL && map_row < board->height) {
            for (int x = 0; x < w && line[x] != '\0'; x++) {
                int idx = map_row * board->width + x; //Row-major math
                if (line[x] == 'X') {
                    board->board[idx].content = 'W';
                }
                else if (line[x] == 'o') {
                    board->board[idx].content= ' ';
                    board->board[idx].has_dot = 1;
                }
                else if (line[x] == '@') {
                    board->board[idx].content = ' ';
                    board->board[idx].has_portal = 1;
                }
                
            }
            map_row++;
            
        }
    }   // Get next line, happens every iteraction
        line = strtok(NULL, "\n");
    }



    board->n_pacmans = 1;

    board->pacmans = calloc(board->n_pacmans, sizeof(pacman_t));
    board->ghosts = calloc(board->n_ghosts, sizeof(ghost_t));

    if (load_pacman_from_file(board, accumulated_points) != 0) {
        // Error handling
        free(buffer);
        return 1;
    }

    load_ghosts_from_file(board);

    free(buffer);
    return 0;
}


int load_pacman_from_file(board_t *board, int accumulated_points) {
    // Open file
    char *pac_file_name = board->pacman_file;
    int fd = open(pac_file_name, O_RDONLY);
    if (fd == -1) {
       
        return 1;
    }
    // Read file into buffer
    char *buffer = read_into_buffer(fd);

    close(fd);

    if (buffer == NULL) {
        return 1;
    }

    // Get first line
    char *line = strtok(buffer, "\n");

    pacman_t *pacman = board->pacmans;

    while (line != NULL) {
        if (line[0] == '#' || line[0] == '\n') {
            // Do nothing, let it reach the bottom to get to the next line
        }
        else if (strncmp(line, "PASSO", 5) == 0) {
            sscanf(line + 5, "%d", &pacman->passo);
        }
        else if (strncmp(line, "POS", 3) == 0) {
            sscanf(line + 3, "%d %d", &pacman->pos_x, &pacman->pos_y);
        }
        line = strtok(NULL, "\n");
    }
    
    board->board[pacman->pos_y * board->width + pacman->pos_x].content = 'P';

    pacman->points = accumulated_points;
    pacman->alive = 1;

    free(buffer);

    return 0;

} 

int load_ghosts_from_file(board_t *board) {}