#ifndef LOADER_H
#define LOADER_H

#include <board.h>
#include <dirent.h>


typedef struct {
    char *level_names[MAX_LEVELS];       // Array of strings (file names)
    int size;        
} LevelList;


void free_level_list(LevelList *list);
int load_level_from_file(board_t *board, const char *full_level_path, int accumulated_points, const char *base_path);
LevelList *load_directory(const char *name);

#endif