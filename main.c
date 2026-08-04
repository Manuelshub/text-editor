#include "editor.h"
#include "terminal_mod.h"
#include "renderer.h"
#include "helper.h"


void process_keypress(editor_t *editor) {
    int key_press;
    char c, *content;
    size_t offset, prev_line_len, doc_len;

    key_press = read_keypress();
    switch (key_press) {
        case CTRL_KEY('q'):
            /* Clear the screen and exit */
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            editor_destroy(editor);
            exit(0);
            break;
        case CTRL_KEY('s'):
            editor_save(editor);
            break;
        case KEY_UP:
            if (editor->cursor.line > 0)
                editor->cursor.line--;
            break;
        case KEY_DOWN:
            if (editor->cursor.line < get_document_line_count(editor))
                editor->cursor.line++;
            break;
        case KEY_LEFT:
            if (editor->cursor.column > 0)
                editor->cursor.column--;
            else if (editor->cursor.line > 0) {
                editor->cursor.line--;
                editor->cursor.column = get_line_length(editor, editor->cursor.line);
            }
            break;
        case KEY_RIGHT:
            if (editor->cursor.column < get_line_length(editor, editor->cursor.line))
                editor->cursor.column++;
            else if (editor->cursor.line < get_document_line_count(editor)) {
                editor->cursor.line++;
                editor->cursor.column = 0;
            }
            break;
        case KEY_BACKSPACE:
            offset = cursor_to_offset(editor);
            if (editor->cursor.column > 0) {
                piece_table_delete(&editor->table, offset - 1, 1);
                editor->cursor.column--;
                editor->dirty = 1;
            } else if (editor->cursor.line > 0) {
           		prev_line_len = get_line_length(editor, editor->cursor.line - 1);
            	piece_table_delete(&editor->table, offset - 1, 1);
             	editor->cursor.line--;
              	editor->cursor.column = prev_line_len;
               	editor->dirty = 1;
            }
            break;
        case KEY_DELETE:
        	content = piece_table_get_content(&editor->table);
        	doc_len = content ? strlen(content) : 0;
         	free(content);
         	offset = cursor_to_offset(editor);
          	if (offset < doc_len) {
         		piece_table_delete(&editor->table, offset, 1);
         		editor->dirty = 1;
           	}
       		break;
        case '\r':
       		offset = cursor_to_offset(editor);
        	piece_table_insert(&editor->table, offset, "\n", 1);
         	editor->cursor.line++;
          	editor->cursor.column = 0;
           	editor->dirty = 1;
            break;
        default:
            if (key_press >= 32 && key_press < 127) {
                offset = cursor_to_offset(editor);
                c = (char)key_press;
                piece_table_insert(&editor->table, offset, &c, 1);
                editor->cursor.column++;
                editor->dirty = 1;
            }
            break;
    }
    if (editor->cursor.line < (size_t)editor->row_offset)
        editor->row_offset = editor->cursor.line;
    if (editor->cursor.line >= (size_t)(editor->row_offset + editor->screen_rows))
        editor->row_offset = editor->cursor.line - editor->screen_rows + 1;
    if (editor->cursor.column >= (size_t)(editor->col_offset + editor->screen_cols))
   		editor->col_offset = editor->cursor.column - editor->screen_cols + 1;
    if (editor->cursor.column < (size_t)editor->col_offset)
   		editor->col_offset = editor->cursor.column;
}

int main(int ac, char **av) {
    editor_t *editor;
    
    editor = editor_init(ac > 1 ? av[1] : NULL);
    if (editor == NULL) {
        fprintf(stderr, "Failed to initialize editor\n");
        return EXIT_FAILURE;
    }
    enable_raw_mode();

    while (1) {
        editor_refresh_screen(editor);
        process_keypress(editor);
    }
    
    return EXIT_SUCCESS;
}