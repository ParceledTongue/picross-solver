/* 
 * Algorithm adapted from rosettacode.org/wiki/Nonogram_solver, primarily the 
 * Java and Python implementations.
 *
 * Line (i.e. bit array) implementation adapted from Dale Hagglund's answer at
 * stackoverflow.com/questions/2633400/c-c-efficient-bit-array. 
 */

#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint32_t word_t;
int BITS_PER_WORD = sizeof(word_t) * 8;

int PUZZLE_HEIGHT = 10;
int PUZZLE_WIDTH = 10;

typedef struct {
    word_t *words;
    int size;
} Line;

typedef struct LineNode {
    Line *line;
    struct LineNode *prev;
    struct LineNode *next;
} LineNode;

typedef struct {
    LineNode *first;
    LineNode *last;
} LineList;

typedef struct {
    int num_rows;
    int num_cols;
    LineList **row_lists;
    LineList **col_lists;
} Board;

typedef struct {
    int *hint;
    int size;
} Hint;

typedef struct {
    int num_rows;
    int num_cols;
    Hint **row_hints;
    Hint **col_hints;
} Hints;

/* === LINE MANIPULATION AND ACCESS === */

Line *line_init(int size) {
    Line *line = malloc(sizeof(Line));
    line->size = size;
    int num_words = size / BITS_PER_WORD + 1;
    line->words = malloc(sizeof(word_t) * num_words);
    // line begins empty
    int i;
    for (i = 0; i < num_words; i++) {
        line->words[i] = 0;
    }

    return line;
}

void line_free(Line *line) {
    free(line->words);
    free(line);
}

inline int bindex(int b) { return b / BITS_PER_WORD; }
inline int boffset(int b) { return b % BITS_PER_WORD; }

void line_set_filled(Line *line, int index) {
    line->words[bindex(index)] |= (1 << (boffset(index)));
}

void line_set_empty(Line *line, int index) {
    line->words[bindex(index)] &= ~(1 << (boffset(index)));
}

bool line_get(Line *line, int index) {
    return line->words[bindex(index)] & (1 << boffset(index));
}

void line_compliment(Line *line) {
    int i, num_words = line->size / BITS_PER_WORD + 1;
    for (i = 0; i < num_words; i++) {
        line->words[i] = ~(line->words[i]);
        if (i == num_words - 1)
            line->words[i] &= ~(~0 << (line->size % BITS_PER_WORD));
    }
}

void line_or(Line *line1, Line *line2) {
    int i, num_words = line2->size / BITS_PER_WORD + 1;
    for (i = 0; i < num_words; i++) {
        line1->words[i] |= line2->words[i];
    }
}

void line_and(Line *line1, Line *line2) {
    int i, num_words = line1->size / BITS_PER_WORD + 1;
    for (i = 0; i < num_words; i++) {
        line1->words[i] &= line2->words[i];
    }
}

void line_shift_left(Line *line) {
    int i, num_words = line->size / BITS_PER_WORD + 1;
    for (i = 0; i < num_words; i++) {
        line->words[i] >>= 1;
        if (i < num_words - 1)
            line->words[i] |= (line->words[i + 1] << (BITS_PER_WORD - 1));
    }
}

void line_shift_right(Line *line) {
    int i, num_words = line->size / BITS_PER_WORD + 1;
    for (i = num_words - 1; i >= 0; i--) {
        line->words[i] <<= 1;
        if (i > 0)
            line->words[i] |= (line->words[i - 1] >> (BITS_PER_WORD - 1));
    }
}

// return new line which is the concatenation of the two inputs
Line *line_concat(Line *line1, Line *line2) {
    int i, new_size = line1->size + line2->size;
    Line *result = line_init(new_size);
    Line *flag = line_init(new_size);
    line_or(result, line1);
    line_or(flag, line2);
    for (i = 0; i < line1->size; i++)
        line_shift_right(flag); // TODO would be good if this could take shamt
    line_or(result, flag);
    line_free(flag);
    return result;
}

void line_print(Line *line) {
    int i;
    for (i = 0; i < line->size; i++) {
        if (line_get(line, i))
            printf("# ");
        else
            printf(". ");
    }
}

/* === LINELIST MANIPULATION AND ACCESS === */

LineList *linelist_init() {
    LineList *linelist = malloc(sizeof(LineList));
    linelist->first = NULL;
    linelist->last = NULL;
}

// Append new line to end of list, returning pointer to the added node
LineNode *linelist_append(LineList *linelist, Line *new_line) {
    LineNode *old_last = linelist->last;
    LineNode *new_last = malloc(sizeof(LineNode));
    if (old_last == NULL)
        linelist->first = new_last;
    else
        old_last->next = new_last;
    new_last->line = new_line;
    new_last->prev = old_last;
    new_last->next = NULL;
    linelist->last = new_last;
    return new_last;
}

void linelist_remove(LineList *linelist, LineNode *linenode) {
    if (linelist->first == linenode)
        linelist->first = linenode->next;
    if (linelist->last == linenode)
        linelist->last = linenode->prev;
    if (linenode->prev != NULL)
        linenode->prev->next = linenode->next;
    if (linenode->next != NULL)
        linenode->next->prev = linenode->prev;
    line_free(linenode->line);
    free(linenode);
}

void linelist_print(LineList *linelist) {
    LineNode *current = linelist->first;
    while (current != NULL) {
        line_print(current->line);
        printf("-> ");
        current = current->next;
    }
    printf("NULL\n");
}

void linelist_free(LineList *linelist) {
    // free each individual line
    while (linelist->first != NULL)
        linelist_remove(linelist, linelist->first);
    // free whole list
    free(linelist);
}

/* === BOARD MANIPULATION AND ACCESS === */

/*
void board_init_row_list(Board *board, int row, LineList *row_list) {
    LineList *cp = &board->row_lists[row];
    cp = row_list;
}

void board_init_col_list(Board *board, int col, LineList *col_list) {
    LineList *cp = &board->col_lists[col];
    cp = col_list;
}
*/

Board *board_init(int num_rows, int num_cols) {
    Board *board = malloc(sizeof(Board));
    board->num_rows = num_rows;
    board->num_cols = num_cols;
    board->row_lists = malloc(num_rows * sizeof(LineList*));
    board->col_lists = malloc(num_cols * sizeof(LineList*));
    return board;
}

void board_free(Board *board) {
    int i;
    for (i = 0; i < board->num_rows; i++) 
        linelist_free(board->row_lists[i]);
    for (i = 0; i < board->num_cols; i++)
        linelist_free(board->col_lists[i]);
    free(board->row_lists);
    free(board->col_lists);
    free(board);
}

/* === FIND ALL POSSIBLE SOLUTIONS FOR GIVEN HINT === */

LineList *generate_segments(LineList *runs, int runs_size, int spaces) {
    // printf("runs_size = %d, spaces = %d\n", runs_size, spaces);
    
    LineList *segments = linelist_init();
    
    if (runs_size == 0) {
        linelist_append(segments, line_init(spaces));
        return segments;
    }

    int lead_spaces;
    for (lead_spaces = 1; lead_spaces <= spaces - runs_size + 1; lead_spaces++) {
        LineList *reduced_runs = linelist_init();
        reduced_runs->first = runs->first->next;
        reduced_runs->last = (runs_size == 1) ? NULL : runs->last;
        LineList *tails = generate_segments(reduced_runs, runs_size - 1, spaces - lead_spaces);
        LineNode *current_node = tails->first;
        while (current_node != NULL) {
            Line *head = line_init(lead_spaces);
            Line *with_run = line_concat(head, runs->first->line);
            Line *with_tail = line_concat(with_run, current_node->line);
            linelist_append(segments, with_tail);
            line_free(head);
            line_free(with_run);
            current_node = current_node->next;
        }
        // TODO free stuff?
    }

    return segments;
}

LineList *get_valid_lines(int *hint, int hint_size, int dim) {
    LineList *valid_lines = linelist_init();

    if (hint_size == 0) {
        linelist_append(valid_lines, line_init(dim));
        return valid_lines;
    }

    LineList *runs = linelist_init();
    int i, hint_sum = 0;
    for (i = 0; i < hint_size; i++) {
        hint_sum += hint[i];
        Line *run = line_init(hint[i]);
        line_compliment(run);
        linelist_append(runs, run);
    }
    
    valid_lines = generate_segments(runs, hint_size, dim + 1 - hint_sum);
    // Get rid of extra space at the beginning of each line
    LineNode *current_node = valid_lines->first;
    while (current_node != NULL) {
        Line *old_line = current_node->line;
        Line *new_line = line_init(dim);
        line_shift_left(old_line);
        line_or(new_line, old_line);
        current_node->line = new_line;
        line_free(old_line);
        current_node = current_node->next;
    }
    
    return valid_lines;
}

/* === REDUCTION === */

typedef struct {
    Line *common_filled;
    Line *common_empty;
} CommonInfo;

CommonInfo *commoninfo_init(int dim) {
    CommonInfo *info = malloc(sizeof(CommonInfo));
    info->common_filled = line_init(dim);
    info->common_empty = line_init(dim);
    line_compliment(info->common_filled);
    return info;
}

void commoninfo_free(CommonInfo *info) {
    line_free(info->common_filled);
    line_free(info->common_empty);
    free(info);
}

CommonInfo *get_common(LineList *lines, int dim) {
    CommonInfo *info = commoninfo_init(dim);
    LineNode *current_node = lines->first;
    while (current_node != NULL) {
        line_and(info->common_filled, current_node->line);
        line_or(info->common_empty, current_node->line);
        current_node = current_node->next;
    }
    return info;
}

int reduce(LineList **a, int a_size, LineList **b, int b_size) {
    int i, j, num_removed = 0;

    for (i = 0; i < a_size; i++) {
        CommonInfo *info = get_common(a[i], b_size);
        # pragma omp parallel for reduction(+:num_removed)
        for (j = 0; j < b_size; j++) {
            LineNode *current_node = b[j]->first;
            while (current_node != NULL) {
                LineNode *next_node = current_node->next;
                if ((line_get(info->common_filled, j) && !line_get(current_node->line, i)) || (!line_get(info->common_empty, j) && line_get(current_node->line, i))) {
                    linelist_remove(b[j], current_node);
                    num_removed++;
                }
                current_node = next_node;
            }
            if (b[j]->first == NULL)
                return -1; // no solution
        }
        commoninfo_free(info);
    }

    return num_removed;
}

int reduce_mutual(Board *board) {
    int cols_removed = reduce(board->row_lists, board->num_rows, board->col_lists, board->num_cols);
    if (cols_removed == -1) return -1; // no solution
    
    int rows_removed = reduce(board->col_lists, board->num_cols, board->row_lists, board->num_rows);
    if (rows_removed == -1) return -1; // no solution

    return cols_removed + rows_removed;
}

/* === HINTS (AND HINT) MANIPULATION AND ACCESS === */

Hint *hint_init(int size) {
    Hint *hint = malloc(sizeof(Hint));
    hint->size = size;
    hint->hint = malloc(size * sizeof(int));
    return hint;
}

void hint_free(Hint *hint) {
    free(hint->hint);
    free(hint);
}

Hints *hints_init(int num_rows, int num_cols) {
    Hints *hints = malloc(sizeof(Hints));
    hints->num_rows = num_rows;
    hints->num_cols = num_cols;
    hints->row_hints = malloc(num_rows * sizeof(Hint*));
    hints->col_hints = malloc(num_cols * sizeof(Hint*));
    return hints;
}

void hints_free(Hints *hints) {
    int i;
    for (i = 0; i < hints->num_rows; i++)
        hint_free(hints->row_hints[i]);
    for (i = 0; i < hints->num_cols; i++)
        hint_free(hints->col_hints[i]);
    free(hints->row_hints);
    free(hints->col_hints);
    free(hints);
}

/* === READ TEXT FILE INTO HINTS STRUCT === */

Hints *read_hints(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    char line[256];
    
    int num_rows;
    int num_cols;
    fgets(line, sizeof(line), file);
    sscanf(line, "%d %d", &num_rows, &num_cols);

    Hints *hints = hints_init(num_rows, num_cols);
    
    int num_read = 0;

    while (fgets(line, sizeof(line), file)) {
        if (!isdigit(line[0])) continue; // ignore non-digit lines
        int i, size = 0;
        int hint_buffer[50];
        // set up buffer
        char *p = strtok(line, " ");
        while (p != NULL) {
            hint_buffer[size++] = atoi(p);
            p = strtok(NULL, " ");
        }
        Hint *hint = hint_init(size);
        for (i = 0; i < size; i++)
            hint->hint[i] = hint_buffer[i];

        if (num_read < num_rows)
            hints->row_hints[num_read] = hint;
        else
            hints->col_hints[num_read - num_rows] = hint;
        num_read++;
    }

    fclose(file);
    return hints;
}

/* === MAIN === */

int main() {
    // Hints *h = hints_init(5, 5);
    // hints_free(h);

    Hints *hints = read_hints("duck.txt");

    printf("%d\n", hints->col_hints[13]->hint[1]);

    Board *board = board_init(3, 3);
    
    int hint_length = 1;
    int *hint1, *hint2, *hint3, *hint4, *hint5, *hint6;
    
    hint1 = malloc(hint_length * sizeof(int));
    hint1[0] = 2;
    board->row_lists[0] = get_valid_lines(hint1, hint_length, 3);

    hint2 = malloc(hint_length * sizeof(int));
    hint2[0] = 1;
    board->row_lists[1] = get_valid_lines(hint2, hint_length, 3);

    hint3 = malloc(hint_length * sizeof(int));
    hint3[0] = 1;
    board->row_lists[2] = get_valid_lines(hint3, hint_length, 3);

    hint4 = malloc(hint_length * sizeof(int));
    hint4[0] = 0;
    board->col_lists[0] = get_valid_lines(hint4, hint_length, 3);

    hint5 = malloc(hint_length * sizeof(int));
    hint5[0] = 1;
    board->col_lists[1] = get_valid_lines(hint5, hint_length, 3);

    hint6 = malloc(hint_length * sizeof(int));
    hint6[0] = 3;
    board->col_lists[2] = get_valid_lines(hint6, hint_length, 3);

    // main loop
    int total_changed;
    do {
        total_changed = reduce_mutual(board);
        if (total_changed == -1) {
            printf("Failed to find a solution.\n");
            return;
        }
    } while (total_changed > 0);

    int i;
    for (i = 0; i < board->num_rows; i++) {
        line_print(board->row_lists[i]->first->line);
        printf("\n");
    }

    /*
    int hint_length = 2;
    int *hint = malloc(hint_length * sizeof(int));
    hint[0] = 2;
    hint[1] = 2;
    LineList *valid_lines = get_valid_lines(hint, hint_length, 5);
    CommonInfo *info = get_common(valid_lines, 5);
    line_print(info->common_empty);
    */
    
    /*
    LineList *runs = linelist_init();
    Line *run1 = line_init(50);
    Line *run2 = line_init(1);
    line_compliment(run1);
    line_compliment(run2);
    linelist_append(runs, run1);
    linelist_append(runs, run2);
    linelist_print(generate_segments(runs, 2, 3));
    */

    /*
    Line *run = line_init(2);
    Line *tail = line_init(2);
    line_set_filled(tail, 1);
    line_compliment(run);
    printf("%" PRIu32 "\n", run->words[0]);
    Line *head = line_init(3);
    Line *with_run = line_concat(head, run);
    Line *with_tail = line_concat(with_run, head);
    // line_print(with_tail);
    
    word_t *w = malloc(sizeof(word_t));
    w[0] = 0;
    w[0] = ~w[0];
    w[0] &= ~(~0 << 1);
    // printf("%" PRIu32 "\n", w[0]);
    */
   
    hints_free(hints);
    board_free(board);
    printf("\n");

    return 0;
}

