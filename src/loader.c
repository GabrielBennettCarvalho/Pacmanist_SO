#include <board.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>




int load_level_from_file(board_t *board, const char *level_path, int accumulated_points) {

    //Open file
    int fd = open(level_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // Get all of the file's statistics (to find its size)
    struct stat file_stat; 
    if (fstat(fd, &file_stat) == -1) {
        close(fd);
        return 1;
    }
    // Get file's size
    off_t file_size = file_stat.st_size;

    // Alocate size (in heap) for the buffer to avoid stack overflows
    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        close(fd);
        return 1;
    }

    // Read the file
    ssize_t bytes_read = read(fd, buffer, file_size);
    if (bytes_read != file_size) {
        // If it's not the same size, something went wrong reading.
        free(buffer);
        close(fd);
        return 1;
    }

    // Null terminate the buffer to treat it like a string.
    buffer[file_size] = '\0';

    // Close the file as we don't need it anymore.
    close(fd);

    char *line = strtok(buffer, "\n");
    /* GUSTAVO: For you my g
    O strtok divide o buffer em tokens, neste caso até encontrar o \n,
    quando o encontrar, devolve tudo para tras dele e fica com um ponteiro '\0'
    a apontar para onde parou, por isso é que quando fazemos strtok(NULL, ...) 
    ele lê onde "estava".
    */

    int w, h, tempo;
    char pac_file[MAX_FILENAME];
    char ghosts_files[MAX_GHOSTS][MAX_FILENAME];
    int ghost_count = 0;
    int map_row = 0;

    

    while (line != NULL) {

        int i = 0;
        
        if (line[0] == '#' || line[0] == '\n') {
            // Go to next line
            line = strtok(NULL, "\n");
            continue;
        }
        // Read Dimensions
        else if (strncmp(line, "DIM", 3) == 0) {
            if (sscanf(line + 3, "%d %d", &w, &h) == 2); { 
                board->width = w;
                board->height = h;
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
           sscanf(line + 3, "%255s", pac_file);
           strcpy(board->pacman_file, pac_file);
        }
        else if (strncmp(line, "MON", 3) == 0) {
           char *ptr = line + 3; // Skip MON
           int offset;
           while (sscanf(ptr, "%s%n", &ghosts_files[ghost_count], &offset) == 1) {
                ptr += offset;
                ghost_count++;
           }
        } else {
            // If it's not a keyword, it's the board
            for (int x = 0; x < w && line[x] != '\0'; x++) {
                if (line[x] == "X") {
                    board->board[map_row * board->width + x].content = 'W';
                }
                else if (line[x] == "o") {
                    board->board[map_row * board->width + x].content= ' ';
                    board->board[map_row * board->width + x].has_dot = 1;
                }
                else if (line[x] == "@") {
                    board->board[map_row * board->width + x].content = ' ';
                    board->board[map_row * board->width + x].has_portal = 1;
                }
                
            }
            map_row++;
            
        }
    }




    board->n_ghosts = ghost_count;
    board->n_pacmans = 1;

    board->board = calloc(board->width * board->height, sizeof(board_pos_t));
    board->pacmans = calloc(board->n_pacmans, sizeof(pacman_t));
    board->ghosts = calloc(board->n_ghosts, sizeof(ghost_t));

    load_pacman_from_file(&board);

    load_ghosts_from_file(&board);

    free(buffer);
    return 0;
}

//TODO;
int load_pacman_from_file(board_t *board) {} // The board already stores the files.

int load_ghosts_from_file(board_t *board) {}