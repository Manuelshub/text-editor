#include "editor.h"
#include "terminal_mod.h"
#include <unistd.h>

/*
 * editor_init - initiailize an editor struct
 * 
 * filename: the name of the file to open in the editor.
 * Return: A pointer to the editor structure.
 */
editor_t *editor_init(const char *filename) {
    char *buffer;
    cursor_t cursor;
    editor_t *editor;
    int open_fd;
    piece_table_t *table;
    struct stat s_buff;
    ssize_t bytes_read, total_read;
    undo_stack_t undo_stack;

    editor = malloc(sizeof(editor_t));
    if (editor == NULL) return NULL;
    
    if (filename != NULL) {
        if (stat(filename, &s_buff) == -1) {
            perror("stat");
            free(editor);
            return NULL;
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
        fprintf(stderr, "Case when the filename to write is not specified\n");
        free(content);
        return -1;
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
