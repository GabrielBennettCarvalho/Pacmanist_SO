#include <board.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>




ssize_t read_line(int fd, char *buf, size_t max) {
    size_t pos = 0;
    char c;

    while (pos < max - 1) {
        ssize_t n = read(fd, &c, 1);
        if ( n <= 0) break; //EOF or error
        buf[pos++] = c;
        if (c == '\n') break ; // end of line
    }

    buf[pos] = '\0';
    return pos;
}

int parse_int(const char *s, int *i) {
    int n = 0;
    int pos = *i;

    while (s[pos] == ' ') pos++;

    while (s[pos] >= '0' && s[pos] <= '9') {
        n = n * 10 + (s[pos] - '0'); // *10 in case the number has more than 1 digit
                                     // subtract by '0' because of ASCII
        pos++;
    }
    *i = pos;
    return n;
}

void parse_string(const char *s, int *i, char *out)
{
    int pos = *i;

    while (s[pos] == ' ')
        pos++;

    int k = 0;

    while (s[pos] != '\n' && s[pos] != '\0') {
        out[k++] = s[pos++];
    }

    out[k] = '\0';

    if (s[pos] == '\n')
        pos++;

    *i = pos;
}





int load_level_from_file(board_t *board, const char *level_path, int accumulated_points) {
    int fd = open(level_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    int w, h, tempo;
    char pacman_file[MAX_FILENAME];
    char ghosts_files[MAX_GHOSTS][MAX_FILENAME];
    int ghost_count = 0;
    char line[256];
    int nbytes;

    while ((read_line(fd, line, sizeof(line))) > 0 ) {

        int i = 0;
        
        if (line[0] == '#')
            continue;
        
        if (strncmp(line, "DIM", 3) == 0) {
            i = 3;
            w = parse_int(line, &i);
            h = parse_int(line, &i);
        }
        if (strncmp(line, "TEMPO", 5) == 0) {
            i = 5;
            tempo = parse_int(line, &i);
        }
        if (strncmp(line, "PAC", 3) == 0) {
            i = 3;
            while (line[i] != '\n') {
                parse_string(line, &i, board->pacman_file);
            }
        }
        if (strncmp(line, "MON", 3) == 0) {
            i = 3;
            while (line[i] != '\n' && line[i] != '\0') {
            parse_string(line, &i, board->ghosts_files[ghost_count]);
            ghost_count++;
        }
        } 
    }


    board->height = h;
    board->width = w;
    board->tempo = tempo;

    board->n_ghosts = ghost_count;
    board->n_pacmans = 1;

    board->board = calloc(board->width * board->height, sizeof(board_pos_t));
    board->pacmans = calloc(board->n_pacmans, sizeof(pacman_t));
    board->ghosts = calloc(board->n_ghosts, sizeof(ghost_t));

    load_pacman_from_file(&board);

    load_ghosts_from_file(&board);

    close(fd);
    return 0;
}

//TODO;
int load_pacman_from_file(board_t *board) {} // The board already stores the files.

int load_ghosts_from_file(board_t *board) {}