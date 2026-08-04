#include "editor.h"

size_t get_document_line_count(editor_t *editor) {
    char *content;
    size_t count, i;

    content = piece_table_get_content(&editor->table);
    if (content == NULL) return 0;

    count = 0;
    for (i = 0; content[i] != '\0'; i++) {
        if (content[i] == '\n')
            count++;
    }
    free(content);
    return count;
}

size_t get_line_length(editor_t *editor, size_t line) {
    char *content, *line_start, *new_line;
    size_t current_line, len;

    content = piece_table_get_content(&editor->table);
    if (content == NULL) return 0;

    current_line = 0;
    line_start = content;
    while((new_line = strchr(line_start, '\n')) != NULL) {
        if (current_line == line) {
            len = new_line - line_start;
            free(content);
            return len;
        }
        current_line++;
        line_start = new_line + 1;
    }
    if (current_line == line) {
        len = strlen(line_start);
        free(content);
        return len;
    }
    free(content);
    return 0;
}
