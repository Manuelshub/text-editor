#include "editor.h"

piece_table_t *piece_table_create(const char *content, size_t length) {
    piece_table_t *table;
    piece_t piece;

    table = malloc(sizeof(piece_table_t));
    if (table == NULL) return NULL;

    if (content != NULL && length > 0) {
        table->original_buffer = malloc(length + 1);
        if (table->original_buffer == NULL) {
            free(table);
            return NULL;
        }
        memcpy(table->original_buffer, content, length);
        table->original_buffer[length] = '\0';
        table->original_size = length;
        table->append_buffer = malloc(INITIAL_CAPACITY);
        if (table->append_buffer == NULL) {
            free(table->original_buffer);
            free(table);
            return NULL;
        }
        table->append_capacity = INITIAL_CAPACITY;
        /* This is zero because append buffer starts at 0, when it reaches 
        append_capacity, we have to reallocate with double the current capacity*/
        table->append_size = 0;
        table->pieces = malloc(INITIAL_CAPACITY * sizeof(piece_t));
        if (table->pieces == NULL) {
            free(table->original_buffer);
            free(table->append_buffer);
            free(table);
            return NULL;
        }
        table->pieces_capacity = INITIAL_CAPACITY;
        table->pieces_count = 0;

        piece.buffer_choice = ORIGINAL;
        piece.start_idx = 0;
        piece.length = length;

        table->pieces[0] = piece;
        table->pieces_count = 1;
    }
    
    else {
        table->original_buffer = NULL;
        table->original_size = 0;
        table->append_buffer = malloc(INITIAL_CAPACITY);
        if (table->append_buffer == NULL) {
            free(table);
            return NULL;
        }
        table->append_capacity = INITIAL_CAPACITY;
        table->append_size = 0;
        
        table->pieces = malloc(INITIAL_CAPACITY * sizeof(piece_t));
        if (table->pieces == NULL) {
            free(table->append_buffer);
            free(table);
            return NULL;
        }
        table->pieces_capacity = INITIAL_CAPACITY;
        table->pieces_count = 0;
    }
    
    return table;
}

void piece_table_destroy(piece_table_t *table) {
    if (table != NULL){
        if (table->original_buffer != NULL)
            free(table->original_buffer);
        free(table->append_buffer);
        free(table->pieces);
        free(table);
    }
}

char *piece_table_get_content(piece_table_t *table) {
    size_t i, total_length, offset;
    char *ret_string, *src;

    total_length = 0;
    offset = 0;
    for (i = 0; i < table->pieces_count; i++)
        total_length += table->pieces[i].length;

    ret_string = malloc(total_length + 1);
    if (ret_string == NULL)
        return NULL;

    for (i = 0; i < table->pieces_count; i++) {
        src = (table->pieces[i].buffer_choice == APPEND)
            ? table->append_buffer
            : table->original_buffer;
        memcpy(ret_string + offset, src + table->pieces[i].start_idx, table->pieces[i].length);
        offset += table->pieces[i].length;
    }
    ret_string[total_length] = '\0';
    return ret_string;
}

int piece_table_insert(piece_table_t *table, size_t offset, const char *text, size_t length) {
    char *new_buff;
    size_t i, running_length, split_pnt;
    piece_t *new_piece_buf, piece_a, piece_b, piece_c;

    /* Make sure to check the capacity and double if needed. */
    if (table->append_size + length > table->append_capacity) {
        while (table->append_size + length > table->append_capacity)
            table->append_capacity *= 2;
        new_buff = realloc(table->append_buffer, table->append_capacity);
        if (new_buff == NULL) return -1;
        table->append_buffer = new_buff;
    }
    /* Move the text into the append buffer */
    memcpy(table->append_buffer + table->append_size, text, length);
    table->append_size += length;

    /* Inserting at the very beginning of the file */
    if (table->pieces_count == 0) {
        piece_b.buffer_choice = APPEND;
        piece_b.start_idx = table->append_size - length;
        piece_b.length = length;
        table->pieces[0] = piece_b;
        table->pieces_count = 1;
        return 0;
    }

    running_length = 0;
    for (i = 0; i < table->pieces_count; i++) {
        if (offset <= running_length + table->pieces[i].length) break;
        running_length += table->pieces[i].length;
    }
    split_pnt = offset - running_length;
    if (table->pieces_count + 2 > table->pieces_capacity) {
        while (table->pieces_count + 2 > table->pieces_capacity)
            table->pieces_capacity *= 2;
        new_piece_buf = realloc(table->pieces, table->pieces_capacity * sizeof(piece_t));
        if (new_piece_buf == NULL) return -1;
        table->pieces = new_piece_buf;
    }
    piece_b.buffer_choice = APPEND;
    piece_b.start_idx = table->append_size - length;
    piece_b.length = length;
    
    if (split_pnt == 0) {
        memmove(&table->pieces[i + 2],
                &table->pieces[i + 1],
                (table->pieces_count - i - 1) * sizeof(piece_t));
        table->pieces[i + 1] = table->pieces[i];
        table->pieces[i] = piece_b;
        table->pieces_count += 1;
    }
    else if (split_pnt == table->pieces[i].length) {
        memmove(&table->pieces[i + 2],
                &table->pieces[i + 1],
                (table->pieces_count -i -1) * sizeof(piece_t));
        table->pieces[i + 1] = piece_b;
        table->pieces_count += 1;
    }
    else {
        /* Shift pieces from index i + 1 onwards, two spots to the right */
        memmove(&table->pieces[i + 3],
                &table->pieces[i + 1],
                (table->pieces_count - i - 1) * sizeof(piece_t));

        piece_a.buffer_choice = table->pieces[i].buffer_choice;
        piece_a.start_idx = table->pieces[i].start_idx;
        piece_a.length = split_pnt;

        piece_c.buffer_choice = table->pieces[i].buffer_choice;
        piece_c.start_idx = table->pieces[i].start_idx + split_pnt;
        piece_c.length = table->pieces[i].length - split_pnt;

        table->pieces[i] = piece_a;
        table->pieces[i + 1] = piece_b;
        table->pieces[i + 2] = piece_c;
        table->pieces_count += 2;
    }
    
    return 0;
}

int piece_table_delete(piece_table_t *table, size_t offset, size_t length) {
    piece_t right, *new_piece_buf;
    size_t i, running_length, i_start, i_end;
    size_t start_split, end_split, del_end, pieces_to_remove;
    
    if (table->pieces_count == 0 || length == 0) return 0;

    running_length = 0;
    i_start = 0;
    i_end = 0;
    del_end = offset + length;
    start_split = 0;
    end_split = 0;

    for (i = 0; i < table->pieces_count; i++) {
        if (offset <= running_length + table->pieces[i].length) {
            i_start = i;
            start_split = offset - running_length;
            break;
        }
        running_length += table->pieces[i].length;
    }

    /* continuing from i_start to find i_end */
    for (i = i_start; i < table->pieces_count; i++) {
        if (del_end <= running_length + table->pieces[i].length) {
            i_end = i;
            end_split = del_end - running_length;
            break;
        }
        running_length += table->pieces[i].length;
    }
    if (i_start == i_end) {
        if(start_split == 0 && end_split == table->pieces[i_start].length) {
            /* delete the entire piece - shift left */
            memmove(&table->pieces[i_start],
                    &table->pieces[i_start + 1],
                    (table->pieces_count - i_start - 1) * sizeof(piece_t));
            table->pieces_count -= 1;
        }
        else if (start_split == 0) {
            /* Truncate from the front */
            table->pieces[i_start].start_idx += end_split;
            table->pieces[i_start].length -= end_split;
        }
        else if (end_split == table->pieces[i_start].length) {
            /* truncate from the back */
            table->pieces[i_start].length = start_split;
        }
        else {
            right.buffer_choice = table->pieces[i_start].buffer_choice;
            right.start_idx = table->pieces[i_start].start_idx + end_split;
            right.length = table->pieces[i_start].length - end_split;
            /* left piece */
            table->pieces[i_start].length = start_split;
            if (table->pieces_count + 1 > table->pieces_capacity) {
                while (table->pieces_count + 1 > table->pieces_capacity)
                    table->pieces_capacity *= 2;
                new_piece_buf = realloc(table->pieces, table->pieces_capacity * sizeof(piece_t));
                if (new_piece_buf == NULL) return -1;
                table->pieces = new_piece_buf;
            }
            /* insert right piece after i_start */
            memmove(&table->pieces[i_start + 2],
                    &table->pieces[i_start + 1],
                    (table->pieces_count - i_start - 1) * sizeof(piece_t));
            table->pieces[i_start + 1] = right;
            table->pieces_count += 1;
        }
    }
    else {
        /* truncate i_start piece */
        table->pieces[i_start].length = start_split;
        /* truncate i_end piece */
        table->pieces[i_end].start_idx += end_split;
        table->pieces[i_end].length -= end_split;
        /* remove pieces between i_start and i_end */
        pieces_to_remove = i_end - i_start - 1;
        if (pieces_to_remove > 0) {
            memmove(&table->pieces[i_start + 1],
                    &table->pieces[i_end],
                    (table->pieces_count - i_end) * sizeof(piece_t));
            table->pieces_count -= pieces_to_remove;
        }
    }

    return 0;
}

size_t cursor_to_offset(editor_t *editor) {
    char *content;
    size_t i, offset, curr_line;

    content = piece_table_get_content(&editor->table);
    if (content == NULL) return 0;

    offset = 0;
    curr_line = 0;
    for (i = 0; content[i] != '\0'; i++) {
        if (curr_line == editor->cursor.line) break;
        if (content[i] == '\n')
            curr_line += 1;
        offset += 1;
    }
    offset += editor->cursor.column;
    free(content);

    return offset;
}

void offset_to_cursor(editor_t *editor, size_t offset) {
	char *content;
	size_t i, line, col;

	content = piece_table_get_content(&editor->table);
	if (content == NULL) return;

	line = 0;
	col = 0;
	for (i = 0; i < offset; i++) {
		if (content[i] == '\n') {
			line += 1;
			col = 0;
		} else col++;
	}
	editor->cursor.line = line;
	editor->cursor.column = col;
}
