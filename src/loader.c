#include <board.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include "loader.h"

/**
 *  ******* TALVEZ COLOCAR EM UM FICHEIRO UTILIDADES ********
 * Função de comparação para qsort. 
 * Ordena os nomes de ficheiro .lvl pela ordem numérica do seu prefixo (ex: '1' em '1.lvl').
 */
int compare_level_numbers(const void *a, const void *b) {
    // 1. Converter os ponteiros genéricos (void*) para char**
    const char *name_a = *(const char **)a;
    const char *name_b = *(const char **)b;

    int num_a, num_b;

    // 2. Extrair o número inicial de cada string.
    if (sscanf(name_a, "%d", &num_a) != 1) {
        // Falha na leitura do número (isto não deve acontecer se os nomes forem válidos)
        return 0; 
    }
    if (sscanf(name_b, "%d", &num_b) != 1) {
        return 0;
    }
    // Se for negativa, 'b' é maior e se for positiva, 'a' é maior.
    return num_a - num_b;
}

//Função privada para inicializar a DynamicList
static DynamicList *init_dynamic_list(int capacity) {
    DynamicList *list = malloc(sizeof(DynamicList));
    if (list == NULL) {
        return NULL;
    }

    list->max_capacity = capacity;
    list->size = 0;
    
    // Alocar o array de ponteiros char*
    list->array = malloc(list->max_capacity * sizeof(char *));
    if (list->array == NULL) {
        free(list); // Liberta a estrutura se o array falhar
        return NULL;
    }
    return list;
}

//Função privada para inicializar o BehaviorMAP
static BehaviorMAP *init_behavior_map(int capacity) {
    BehaviorMAP *map = malloc(sizeof(BehaviorMAP));
    if (map == NULL) {
        return NULL;
    }

    map->max_capacity = capacity;
    map->size = 0;

    // Alocar o array de estruturas BehaviorEntry
    map->array = malloc(map->max_capacity * sizeof(BehaviorEntry));
    if (map->array == NULL) {
        free(map); // Liberta a estrutura se o array falhar
        return NULL;
    }
    return map;
}

// Função auxiliar para a limpeza completa, usada se ocorrer um erro no loop
void cleanup_resources(DIR *dir, GameResources *res) {
    if (dir != NULL) closedir(dir);
    if (res != NULL) {

        // Limpeza da DynamicList
        if (res->levels != NULL) {
            // Se houver nomes de ficheiro alocados, tem de os libertar individualmente
            for (int i = 0; i < res->levels->size; i++) {
                free(res->levels->array[i]);
            }
            free(res->levels->array);
            free(res->levels);
        }

        // Limpeza do BehaviorMap
        if (res->behaviors != NULL) {
            for (int i = 0; i < res->behaviors->size; i++) {
                // Liberta as strings da Chave do mapa (Deep Copy)
                free(res->behaviors->array[i].file_name);
            }
            free(res->behaviors->array); // Liberta o array entries de BehaviorEntry
            free(res->behaviors);        // Liberta a estrutura BehaviorMap
        }
        free(res);
    }
}

GameResources *load_directory(const char *name){

    //abrir diretório
    DIR *directory = opendir(name);
    if(directory ==NULL){
        perror("opendir");
        return NULL;
    }

    //ler entradas do diretório
    struct dirent *entrada;

    //inicializar estrutura para guardar recursos do jogo
    GameResources *resources = malloc(sizeof(GameResources));
    if(resources == NULL){
        closedir(directory);
        return NULL;
    }

    //iniciallizar listas dentro da estrutura, usando funçao privada
    resources->levels = init_dynamic_list(10);
    if(resources->levels == NULL){
        free(resources->levels);
        free(resources);
        closedir(directory);
        return NULL;
    }
    DynamicList *files = resources->levels;


    //inicializar mapa de behaviors, usando função privada
    resources->behaviors = init_behavior_map(5);
    if(resources->behaviors == NULL){
        cleanup_resources(directory, resources);
        return NULL;
    }
    BehaviorMAP *map = resources->behaviors;
   

    while((entrada = readdir(directory)) != NULL){

        if(strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0){
            continue;
        }

        const char *file_name = entrada->d_name;
        int len = strlen(file_name);

        //verificar se o ficheiro é .lvl
        if(len > 4 && strcmp(file_name + (len -4), ".lvl") == 0){

            //realocar memoria se necessário
            if(files->size >= files->max_capacity){
                files->max_capacity *= 2;
                //usar o temporary_array vai garantir que não perdemos o ponteiro original em caso de falha
                //assim evitamos a perda de dados e potenciais memory leaks
                char **temporary_array = realloc(files->array, files->max_capacity * sizeof(char*));
                if(temporary_array == NULL){
                    cleanup_resources(directory, resources);
                    return NULL;
                }
                //depois de garantir que a realocação foi bem sucedida, atualizar o ponteiro
                files->array = temporary_array;
            }

            char *new_name = malloc(len+1);
            // temos que fazer a limpeza de tudo, pq não podemos retornar o resultado incompleto
            if(new_name == NULL){
                cleanup_resources(directory, resources);
                return NULL;
            }
            strcpy(new_name, file_name);


            //adicionar o nome do ficheiro à lista de
            files->array[files->size] = new_name;
            files->size++;
            
            //verificar se o ficheiro é .p ou .m
        }else if(len > 2 && (strcmp(file_name +(len-2), ".p")==0 || strcmp(file_name +(len-2), ".m")== 0)){
            // verificar se é necessário realocar memória
            if(map->size >= map->max_capacity){
                map->max_capacity *=2;
                // realloc seguro (variável temporária)
                BehaviorEntry *temporary_array = realloc (map->array, map->max_capacity * sizeof(BehaviorEntry));
                if(temporary_array == NULL){
                    cleanup_resources(directory, resources);
                    return NULL;
                }
                map->array = temporary_array;
            }

            //criar cópia do nome do ficheiro
            char *new_behavior_name = malloc(len +1);
            if(new_behavior_name == NULL){
                cleanup_resources(directory, resources);
                return NULL;    
            }
            
            strcpy(new_behavior_name, file_name);
            //adicionar o nome do ficheiro à lista de behaviors
            map->array[map->size].file_name = new_behavior_name;

            map->size++;

        }
    }

    //ordenar a lista de níveis numericamente 1.lvl, 2.lvl, 3.lvl , etc
    qsort(files->array, files->size, sizeof(char *), compare_level_numbers);  

    closedir(directory);

    return resources;
}

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

int load_ghosts_from_file(board_t *board) {
    
}