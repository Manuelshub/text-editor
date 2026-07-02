#ifndef TERMINAL_MOD_H
#define TERMINAL_MOD_H


#include <termios.h>
#include <sys/ioctl.h>
#include "editor.h"

typedef enum {
    KEY_UP = 1000,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_DELETE,
    KEY_BACKSPACE
} special_key_t;

void enable_raw_mode(void);
void restore_terminal(void);
int read_keypress(void);
int get_window_size(int *rows, int *cols);
void editor_draw_rows(editor_t *editor, char *buf, int *buf_len);
void editor_refresh_screen(editor_t *editor);
void buffer_append(char *buf, int *buf_len, const char *str, int len);


#endif /* TERMINAL_MOD_H */
