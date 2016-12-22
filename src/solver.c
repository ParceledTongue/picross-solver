/* 
 * Algorithm adapted from rosettacode.org/wiki/Nonogram_solver, primarily the 
 * Java and Python implementations.
 *
 * Line (i.e. bit array) implementation adapted from Dale Hagglund's answer at
 * stackoverflow.com/questions/2633400/c-c-efficient-bit-array. 
 */

#include <ctype.h>
#include <inttypes.h>
#include <omp.h>
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

LineList *generate_segments(LineNode *first_run, int runs_size, int spaces) {
    LineList *segments = linelist_init();
    
    if (runs_size == 0) {
        linelist_append(segments, line_init(spaces));
        return segments;
    }

    int lead_spaces;
    for (lead_spaces = 1; lead_spaces <= spaces - runs_size + 1; lead_spaces++) {
        LineList *tails = generate_segments(first_run->next, runs_size - 1, spaces - lead_spaces);
        // TODO free reduced_runs
        LineNode *current_node = tails->first;
        while (current_node != NULL) {
            Line *head = line_init(lead_spaces);
            Line *with_run = line_concat(head, first_run->line);
            Line *with_tail = line_concat(with_run, current_node->line);
            linelist_append(segments, with_tail);
            line_free(head);
            line_free(with_run);
            current_node = current_node->next;
        }
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
    
    valid_lines = generate_segments(runs->first, hint_size, dim + 1 - hint_sum);
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
    bool failed = false;

    for (i = 0; i < a_size &&! failed; i++) {
        CommonInfo *info = get_common(a[i], b_size);
        # pragma omp parallel for reduction(+:num_removed) schedule(dynamic)
        for (j = 0; j < b_size; j++) {
            # pragma omp flush(failed)
            if (!failed) {
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
                    failed = true; // no solution
            }
        }
        commoninfo_free(info);
    }

    return failed ? -1 : num_removed;
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

int main(int argc, char *argv[]) {
    if (argc > 2) {
        printf("Too many arguments supplied.\n");
        return -1;
    } else if (argc < 2) {
        printf("Argument expected.\n");
        return -1;
    }

    Hints *hints = read_hints(argv[1]);
    Board *board = board_init(hints->num_rows, hints->num_cols);
   
    double start_time = omp_get_wtime();
    // calculate and store all permutations for each line
    int i, nthread, maxthreads = omp_get_max_threads();
    double *time = malloc(maxthreads * sizeof(double));
    int *iterations = malloc(maxthreads * sizeof(int));
    memset(time, 0, maxthreads * sizeof(double));
    memset(iterations, 0, maxthreads * sizeof(int));
    # pragma omp parallel for reduction(max:nthread) schedule(dynamic)
    for (i = 0; i < hints->num_rows + hints->num_cols; i++) {
        double line_start_time = omp_get_wtime();
        int index = (i < hints->num_rows) ? i : i - hints->num_rows;
        if (i < hints->num_rows)
            board->row_lists[index] = get_valid_lines(hints->row_hints[index]->hint, hints->row_hints[index]->size, hints->num_cols);
        else
            board->col_lists[index] = get_valid_lines(hints->col_hints[index]->hint, hints->col_hints[index]->size, hints->num_rows);
        double line_finish_time = omp_get_wtime();
        nthread = omp_get_thread_num();
        time[nthread] += line_finish_time - line_start_time;
        iterations[nthread]++;
    }
    for (i = 0; i <= nthread; i++) {
        printf("Thread %d worked for %f seconds on %d iterations.\n", i, time[i], iterations[i]);
    }
    free(time);
    free(iterations);
    double permutation_finish_time = omp_get_wtime();
    
    // main loop
    int total_changed;
    do {
        total_changed = reduce_mutual(board);
        if (total_changed == -1) {
            printf("Failed to find a solution.\n");
            return -1;
        }
    } while (total_changed > 0);
    double solve_finish_time = omp_get_wtime();

    for (i = 0; i < board->num_rows; i++) {
        line_print(board->row_lists[i]->first->line);
        printf("\n");
    }
    

    hints_free(hints);
    board_free(board);
    printf("%f seconds to calculate permutations.\n%f seconds to find the solution.\n%f seconds total.\n\n", permutation_finish_time - start_time, solve_finish_time - permutation_finish_time, solve_finish_time - start_time);

    return 0;
}

