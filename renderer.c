#include "renderer.h"
#include "helper.h"
#include <stdio.h>

/*
 * buf_append - Helper function to append to a buffer.
 */
void buffer_append(char *buf, int *buf_len, const char *str, int len) {
    memcpy(buf + *buf_len, str, len);
    *buf_len += len;
}

/*
 * get_window_size - Get the current window size
 * 
 * rows: pointer to the actual row of our editor.
 * cols: pointer to the column of our editor.
 * 
 * Return: -1 on failure and 0 on success.
 */
int get_window_size(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        perror("ioctl");
        return -1;
    }
    
    *rows = ws.ws_row;
    *cols = ws.ws_col;

    return 0;
}

void editor_draw_rows(editor_t *editor, char *buf, int *buf_len) {
    char *content, *newline, *line_start;
    char *line_starts[5000], *line;
    int line_lengths[5000];
    int total_lines, i, doc_line, line_len, pre_len;
    int match_draw_len, post_len;

    content = piece_table_get_content(&editor->table);
    if (content == NULL) return;

    total_lines = 0;
    line_start = content;
    while ((newline = strchr(line_start, '\n')) != NULL) {
        line_starts[total_lines] = line_start;
        line_lengths[total_lines] = newline - line_start;
        total_lines++;
        line_start = newline + 1;
    }
    if (*line_start != '\0') {
        line_starts[total_lines] = line_start;
        line_lengths[total_lines] = strlen(line_start);
        total_lines++;
    }

    for (i = 0; i < editor->screen_rows - 1; i++) {
        doc_line = i + editor->row_offset;

        if (doc_line < total_lines) {
        	line = line_starts[doc_line];
            line_len = line_lengths[doc_line];

            if (editor->col_offset < line_len) {
            	line += editor->col_offset;
             	line_len -= editor->col_offset;
            } else
           		line_len = 0;
            if (line_len > editor->screen_cols)
                line_len = editor->screen_cols;

            if (editor->search_active && (size_t)doc_line == editor->match_line) {
            	pre_len = (int)editor->match_col - editor->col_offset;
             	if (pre_len < 0) pre_len = 0;
              	if (pre_len > line_len) pre_len = line_len;

               	match_draw_len = (int)editor->match_len;
                if (pre_len + match_draw_len > line_len) {
              		match_draw_len = line_len - pre_len;
                }

                post_len = line_len - pre_len - match_draw_len;

                /* draw before match */
                buffer_append(buf, buf_len, line, pre_len);
                /* draw match highlighted */
                buffer_append(buf, buf_len, "\x1b[7m", 4);
                buffer_append(buf, buf_len, line + pre_len, match_draw_len);
                buffer_append(buf, buf_len, "\x1b[m", 3);
                /* draw after match */
                buffer_append(buf, buf_len, line + pre_len + match_draw_len, post_len);
            }
            else 
            	buffer_append(buf, buf_len, line, line_len);
        }
        else {
            buf[*buf_len] = '~';
            *buf_len += 1;
        }
        /* clear to end of line then move to next */
        buffer_append(buf, buf_len, "\x1b[K", 3);
        if (i < editor->screen_rows - 1)
        	buffer_append(buf, buf_len, "\r\n", 2);
    }
    
    free(content);
}

/*
 * editor_refresh_screen - 
 */
void editor_refresh_screen(editor_t *editor) {
    char buf[RENDER_BUF_SIZE], seq[32];
    int buf_len, seq_len;
    int screen_row, screen_col;

    buf_len = 0;
    /* Hide cursor */
    buffer_append(buf, &buf_len, "\x1b[?25l", 6);
    /* Move to top left */
    buffer_append(buf, &buf_len, "\x1b[H", 3);

    /* draw rows */
    editor_draw_rows(editor, buf, &buf_len);
    editor_draw_status_bar(editor, buf, &buf_len);

    screen_row = (int)editor->cursor.line - editor->row_offset + 1;
    screen_col = (int)editor->cursor.column - editor->col_offset + 1;
    if (screen_row < 1) screen_row = 1;
    if (screen_col < 1) screen_col = 1;
    /* Move cursor to correct position */
    seq_len = snprintf(seq, sizeof(seq), "\x1b[%d;%dH", screen_row, screen_col);
    buffer_append(buf, &buf_len, seq, seq_len);
    /* Show cursor */
    buffer_append(buf, &buf_len, "\x1b[?25h", 6);
    /* Write everything at once */
    write(STDOUT_FILENO, buf, buf_len);
}

/*
 * editor_draw_status_bar - Editor status bar
 * 
 * editor: pointer to an editor struct
 * buf: the buffer to be drawn
 * buf_len: the length of the buffer to be drawn.
 */
void editor_draw_status_bar(editor_t *editor, char *buf, int *buf_len) {
	char status[256], rstatus[64];
	int len, rlen, st_len;

	if (editor->status_msg[0] != '\0') {
		buffer_append(buf, buf_len, "\x1b[7m", 4);
		st_len = strlen(editor->status_msg);
		if (st_len > editor->screen_cols)
			st_len = editor->screen_cols;
		buffer_append(buf, buf_len, editor->status_msg, st_len);
		while (st_len < editor->screen_cols) {
			buffer_append(buf, buf_len, " ", 1);
			st_len++;
		}
		buffer_append(buf, buf_len, "\x1b[m", 3);
		return; /* Skip normal status bar */
	}
	/* Turn on reverse video */
	buffer_append(buf, buf_len, "\x1b[7m", 4);

	/* Left side - filename, lines, dirty */
	len = snprintf(status, sizeof(status), " %.20s - %zu lines %s",
		editor->filename ? editor->filename : "[No Name]",
		get_document_line_count(editor),
		editor->dirty ? "[Modified]" : "");

	/* Right side cursor position */
	rlen = snprintf(rstatus, sizeof(rstatus), "ln %zu, col %zu", 
		editor->cursor.line + 1,
		editor->cursor.column + 1);

	/* append left side */
    if (len > editor->screen_cols) len = editor->screen_cols;
    buffer_append(buf, buf_len, status, len);
	
    /* fill middle with spaces */
    while (len < editor->screen_cols) {
        if (editor->screen_cols - len == rlen) {
            buffer_append(buf, buf_len, rstatus, rlen);
            break;
        }
        buffer_append(buf, buf_len, " ", 1);
        len++;
    }

	/* turn off attributes */
    buffer_append(buf, buf_len, "\x1b[m", 3);
}
