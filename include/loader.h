#ifndef LOADER_H
#define LOADER_H

#include <board.h>
#include <dirent.h>

// Not sure if we need to create 2 whole new files for this.

// Dynamic list structure to hold level file names
typedef struct{
    char **array;
    int size;
    int max_capacity;
}DynamicList;

// Structure to hold behavior entry
typedef struct{
    char *file_name;
    // char * content; o behavior dos .m e dos .p podem ser adicionados aqui
    /*// mais tarde faria assim para encontrar o comportamento associado a uma chave
    BehaviorEntry *find_behavior_entry(BehaviorMap *map, const char *key) {
        for (int i = 0; i < map->size; i++) {
            if (strcmp(map->entries[i].key, key) == 0) {
                return &map->entries[i];
            }
        }
        return NULL; // Não encontrado
}*/
}BehaviorEntry;

// Map structure to hold behavior entries
typedef struct{
    BehaviorEntry *array;
    int size;
    int max_capacity;

}BehaviorMAP;

// Structure to hold all game resources
typedef struct{
    DynamicList *levels;
    BehaviorMAP *behaviors;
}GameResources;


void cleanup_resources(GameResources *res);
int load_level_from_file(board_t *board, const char *level_path, int accumulated_points);
GameResources *load_directory(const char *name);

#endif