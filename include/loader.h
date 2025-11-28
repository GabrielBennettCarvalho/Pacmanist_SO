#ifndef LOADER_H
#define LOADER_H

#include <board.h>

// Not sure if we need to create 2 whole new files for this.

int load_level_from_file(board_t *board, const char *level_path, int accumulated_points);

#endif