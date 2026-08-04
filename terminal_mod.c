#include "terminal_mod.h"

static struct termios original_termios;

void restore_terminal(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1)
        perror("tcsetattr");
}

void enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    atexit(restore_terminal);

    struct termios raw = original_termios;
    raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP 
                    | INLCR | IGNCR | ICRNL | IXON);
    raw.c_oflag &= ~OPOST;
    raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

/*
 * read_keypress - This is function is used to map special keys.
 */
int read_keypress(void) {
    char c, seq[3];
    
    if (read(0, &c, 1) != 1) return -1;
    if ((unsigned char)c == 127) return KEY_BACKSPACE;
    if (c != '\x1b') return c;
    else {
        if (read(0, &seq[0], 1) != 1) return '\x1b';
        if (read(0, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                case '3': 
                    if (read(0, &seq[2], 1) != 1) return '\x1b';
                    if (seq[2] == '~') return KEY_DELETE;
                    break;
                case '5':
                    if (read(0, &seq[2], 1) != 1) return '\x1b';
                    if (seq[2] == '~') return KEY_PAGE_UP;
                    break;
                case '6':
                    if (read(0, &seq[2], 1) != 1) return '\x1b';
                    if (seq[2] == '~') return KEY_PAGE_DOWN;
                    break;
            }
        }
    }
    return '\x1b';
}
