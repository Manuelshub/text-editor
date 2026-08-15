#include "editor.h"
#include "renderer.h"
#include "terminal_mod.h"

/*
 * editor_init - initiailize an editor and returns a pointer to an editor struct
 * 
 * filename: the name of the file to open in the editor.
 * Return: A pointer to the editor structure.
 */
editor_t *editor_init(const char *filename) {
    char *buffer;
    cursor_t cursor;
    editor_t *editor;
    int open_fd, s_flags;
    piece_table_t *table;
    struct stat s_buff;
    ssize_t bytes_read, total_read;
    undo_stack_t undo_stack;

    editor = malloc(sizeof(editor_t));
    if (editor == NULL) return NULL;

    s_flags = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    if (filename != NULL) {
        if (stat(filename, &s_buff) == -1) {
        	if (errno == ENOENT) {
         		if (creat(filename, s_flags) == -1) {
           			perror("creat");
            		free(editor);
              		return NULL;
           		}
         	} else {
         		perror("stat");
            	free(editor);
             	return NULL;
          	}
        }
        open_fd = open(filename, O_RDONLY);
        if (open_fd == -1) {
            perror("open");
            free(editor);
            return NULL;
        }
        buffer = malloc((size_t)s_buff.st_size + 1);
        if (buffer == NULL) {
            close(open_fd);
            free(editor);
            return NULL;
        }

        total_read = 0;
        while (total_read < s_buff.st_size) {
            bytes_read = read(open_fd, buffer + total_read, s_buff.st_size - total_read);
            if (bytes_read == -1) {
                perror("read");
                close(open_fd);
                free(buffer);
                free(editor);
                return NULL;
            }
            if (bytes_read == 0) break;
            total_read += bytes_read;
        }
        buffer[total_read] = '\0';
        close(open_fd);
        table = piece_table_create(buffer, s_buff.st_size);
        if (table == NULL) {
            free(buffer);
            free(editor);
            return NULL;
        }
        free(buffer);
        editor->filename = strdup(filename);
        if (editor->filename == NULL) {
            free(table);
            return NULL;
        }
    }
    else {
        table = piece_table_create(NULL, 0);
        if (table == NULL) {
            free(editor);
            return NULL;
        }
        editor->filename = NULL;
    }
    cursor.line = 0;
    cursor.column = 0;
    editor->dirty = 0;
    editor->table = *table;
    editor->cursor = cursor;
    if (get_window_size(&editor->screen_rows, &editor->screen_cols) == -1) {
        editor_destroy(editor);
        return NULL;
    }
    editor->row_offset = 0;
    editor->col_offset = 0;
    editor->status_msg[0] = '\0';
    editor->search_active = 0;
    editor->match_line = 0;
    editor->match_len = 0;
    editor->match_col = 0;

    undo_stack.entries = NULL;
    undo_stack.top = 0;
    undo_stack.capacity = 0;
    editor->undo_stack = undo_stack;

    free(table);
    return editor;
}

int editor_save(editor_t *editor) {
    char *content;
    int open_fd, flags;
    size_t content_len;
    ssize_t bytes_written, total_written;

    flags = O_WRONLY | O_CREAT | O_TRUNC;
    content = piece_table_get_content(&editor->table);
    if (content == NULL) return -1;

    if (editor->filename == NULL) {
    	editor->filename = editor_prompt_filename(editor);
     	if (editor->filename == NULL) {
        	free(content);
         	return -1;
      	}
    }
    open_fd = open(editor->filename, flags, 0644);
    if (open_fd == -1) {
        perror("open");
        free(content);
        return -1;
    }

    content_len = strlen(content);
    total_written = 0;
    while ((size_t)total_written < content_len) {
        bytes_written = write(open_fd, content + total_written, content_len - total_written); 
        if (bytes_written == -1) {
            perror("write");
            close(open_fd);
            free(content);
            return -1;
        }
        total_written += bytes_written;
    }
    close(open_fd);
    editor->dirty = 0;
    
    free(content);
    return 0;
}

void editor_destroy(editor_t *editor) {
    size_t i;

    if (editor == NULL) return;

    /* free piece table internals */
    free(editor->table.original_buffer);
    free(editor->table.append_buffer);
    free(editor->table.pieces);

    /* free undo stack */
    if (editor->undo_stack.entries != NULL) {
        for (i = 0; i < editor->undo_stack.top; i++)
            free(editor->undo_stack.entries[i].pieces);
        free(editor->undo_stack.entries);
    }

    free(editor->filename);
    free(editor);
}

void editor_search(editor_t *editor) {
	char query[256], *content, *match;
	int query_len, key;
	size_t saved_line, saved_col, offset;

	query[0] = '\0';
	query_len = 0;
	saved_line = editor->cursor.line;
	saved_col = editor->cursor.column;
	while (1) {
		key = read_keypress();
		if (key == '\x1b') {
            /* escape — restore cursor */
            editor->cursor.line = saved_line;
            editor->cursor.column = saved_col;
            editor->status_msg[0] = '\0';
            editor->search_active = 0;
            break;
        } else if (key == '\r') {
            /* enter — confirm and exit search */
            editor->status_msg[0] = '\0';
            break;
        } else if (key == KEY_BACKSPACE) {
            if (query_len > 0)
                query[--query_len] = '\0';
        } else if (key >= 32 && key < 127) {
            if (query_len < 255) {
                query[query_len++] = (char)key;
                query[query_len] = '\0';
            }
        }
		/* draw search prompt in status bar */
		snprintf(editor->status_msg, sizeof(editor->status_msg),
			"Search: %s", query);
		
	 	/* search on every keystroke */
        if (query_len > 0) {
            content = piece_table_get_content(&editor->table);
            if (content) {
                match = strstr(content, query);
                if (match) {
                    offset = match - content;
                    offset_to_cursor(editor, offset);
                    editor->search_active = 1;
                    editor->match_line = editor->cursor.line;
                    editor->match_col = editor->cursor.column;
                    editor->match_len = query_len;
                } else {
                    snprintf(editor->status_msg, sizeof(editor->status_msg),
                    	"Search: %s [No match]", query);
                }
                free(content);
            }
        } else {
            editor->search_active = 0;
        }
        editor_refresh_screen(editor);
	}
}

char *editor_prompt_filename(editor_t *editor) {
	char filename[256];
	int filename_len, key;

	filename[0] = '\0';
	filename_len = 0;

	while (1) {
		snprintf(editor->status_msg, sizeof(editor->status_msg),
			"Save as: %s", filename);
		editor_refresh_screen(editor);

		key = read_keypress();
		if (key == '\x1b') {
			editor->status_msg[0] = '\0';
			return NULL;
		} else if (key == '\r') {
			editor->status_msg[0] = '\0';
			if (filename_len > 0)
				return strdup(filename);
		} else if (key == KEY_BACKSPACE) {
			if (filename_len > 0)
				filename[--filename_len] = '\0';
		} else if (key >= 32 && key < 127) {
			if (filename_len < 255) {
				filename[filename_len++] = (char)key;
				filename[filename_len] = '\0';
			}
		}
	}
	return NULL;
}
