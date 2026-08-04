#ifndef RENDERER_H
#define RENDERER_H

#include <sys/ioctl.h>
#include "editor.h"

#define RENDER_BUF_SIZE 65536

void buffer_append(char *buf, int *buf_len, const char *str, int len);
int get_window_size(int *rows, int *cols);
void editor_draw_rows(editor_t *editor, char *buf, int *buf_len);
void editor_refresh_screen(editor_t *editor);

#endif /* RENDERER_H */