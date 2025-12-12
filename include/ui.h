#ifndef UI_H
#define UI_H

void *ui_thread_func(void *arg);

void screen_refresh(board_t * game_board, int mode);

#endif