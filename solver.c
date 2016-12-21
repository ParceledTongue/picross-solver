/* 
 * Algorithm adapted from rosettacode.org/wiki/Nonogram_solver, primarily the 
 * Java and Python implementations.
 *
 * Line (i.e. bit array) implementation adapted from Dale Hagglund's answer at
 * stackoverflow.com/questions/2633400/c-c-efficient-bit-array. 
 */

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
    int numRows;
    int numCols;
    LineList *rowLists;
    LineList *colLists;
} Board;

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

void line_or(Line *line1, Line *line2) {
    int i, num_words = line1->size / BITS_PER_WORD + 1;
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

void line_shift_right(Line *line) {
    int i, num_words = line->size / BITS_PER_WORD + 1;
    for (i = num_words - 1; i >= 0; i--) {
        line->words[i] <<= 1;
        if (i > 0)
            line->words[i] |= (line->words[i - 1] >> (BITS_PER_WORD - 1));
    }
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

/* === FIND ALL POSSIBLE SOLUTIONS FOR GIVEN HINT === */

int ipow(int base, int exp) {
    int result = 1;
    while (exp) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

LineList *get_valid_lines(int *hint, int hint_size, int dim) {
    LineList *valid_lines = linelist_init();

    if (hint_size == 0) {
        linelist_append(valid_lines, line_init(dim));
        return valid_lines;
    }

    Line *blocks[hint_size];
    int i, j, start_pos = 0;
    for (i = 0; i < hint_size; i++) {
        blocks[i] = line_init(dim);
        for (j = 0; j < hint[i]; j++)
            line_set_filled(blocks[i], j + start_pos);
        start_pos += hint[i] + 1;
    }
    
    long iteration = 0;
    bool done = false;
    while (!done) {
        Line *together = line_init(dim);
        int base_shift = 0;
        for (i = 0; i < hint_size; i++) {
            Line *shifted_block = blocks[i]; // TODO make this a deep copy
            int shift = base_shift + (iteration / ipow(hint_size, hint_size - i - 1)) % hint_size;
            for (j = 0; j < shift; j++)
                line_shift_right(shifted_block);
            base_shift = shift;
            line_or(together, shifted_block);
        }
        linelist_append(valid_lines, together);
        iteration++;
        done = (iteration == 4);
    }
    
    for (i = 0; i < hint_size; i++) {
        line_free(blocks[i]);
    }

    return valid_lines;
}

/* === MAIN === */

int main() {
    int hint_length = 3;
    int *hint = malloc(hint_length * sizeof(int));
    hint[0] = 1;
    hint[1] = 2;
    hint[2] = 44;
    linelist_print(get_valid_lines(hint, hint_length, 50));

    return 0;
}

