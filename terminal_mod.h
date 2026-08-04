#ifndef TERMINAL_MOD_H
#define TERMINAL_MOD_H

#define CTRL_KEY(k) ((k) & 0x1f)

#include <termios.h>
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


#endif /* TERMINAL_MOD_H */
