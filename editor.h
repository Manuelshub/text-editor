#ifndef EDITOR_H
#define EDITOR_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* Initial capacity for any buffer that would require reallocation */
#define INITIAL_CAPACITY 64
#define RENDER_BUF_SIZE 65536

typedef enum BUFFER {
    ORIGINAL,
    APPEND
} choice_t;

/* Piece struct */
typedef struct piece {
    choice_t buffer_choice;
    size_t start_idx;
    size_t length;
} piece_t;

typedef struct cursor {
    size_t line;
    size_t column;
} cursor_t;

typedef struct undo_entry {
    piece_t *pieces;
    size_t pieces_count;
} undo_entry_t;

typedef struct undo_stack {
    undo_entry_t *entries;
    size_t top;
    size_t capacity;
} undo_stack_t;

/* Array of pieces called piece table with other fields */
typedef struct piece_table {
    piece_t *pieces;
    char *original_buffer;
    char *append_buffer;
    size_t pieces_count;
    size_t pieces_capacity;
    size_t append_size;
    size_t append_capacity;
    size_t original_size;
} piece_table_t;

typedef struct editor {
    piece_table_t table;
    cursor_t cursor;
    char *filename;
    int dirty;
    undo_stack_t undo_stack;
    int screen_rows;    /* terminal height */
    int screen_cols;    /* terminal width */
    int row_offset;
    int col_offset;
} editor_t;

piece_table_t *piece_table_create(const char *content, size_t length);
int piece_table_insert(piece_table_t *table, size_t offset, const char *text, size_t length);
int piece_table_delete(piece_table_t *table, size_t offset, size_t length);
char *piece_table_get_content(piece_table_t *table);
void piece_table_destroy(piece_table_t *table);
editor_t *editor_init(const char *filename);
int editor_save(editor_t *editor);
void editor_destroy(editor_t *editor);
size_t cursor_to_offset(editor_t *editor);



#endif  /* EDITOR_H */
