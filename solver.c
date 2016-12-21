/* Line (i.e. bit array) implementation inspired by Dale Hagglund's answer at
 * stackoverflow.com/questions/2633400/c-c-efficient-bit-array. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned long word_t;
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

void line_free(Line* line) {
    free(line->words);
    free(line);
}

inline int bindex(int b) { return b / BITS_PER_WORD; }
inline int boffset(int b) { return b % BITS_PER_WORD; }

void line_set_filled(Line* line, int index) {
    line->words[bindex(index)] |= (1 << (boffset(index)));
}

void line_set_empty(Line* line, int index) {
    line->words[bindex(index)] &= ~(1 << (boffset(index)));
}

bool line_get(Line* line, int index) {
    return line->words[bindex(index)] & (1 << boffset(index));
}

void line_print(Line* line) {
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

// Add new line to end of list, returning pointer to the added node
LineNode *linelist_add(LineList *linelist, Line *new_line) {
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

/* === MAIN === */

int main() {
    LineList *linelist = linelist_init();
    
    Line *line0 = line_init(10);
    Line *line1 = line_init(10);
    Line *line2 = line_init(10);
    
    line_set_filled(line0, 0);
    line_set_filled(line1, 1);
    line_set_filled(line2, 2);
    
    LineNode *node0 = linelist_add(linelist, line0);
    LineNode *node1 = linelist_add(linelist, line1);
    LineNode *node2 = linelist_add(linelist, line2);
    
    linelist_print(linelist);
    linelist_remove(linelist, node0);
    linelist_print(linelist);
    linelist_remove(linelist, node1);
    linelist_print(linelist);
    linelist_remove(linelist, node2);
    linelist_print(linelist);
    linelist_free(linelist);
    
    return 0;
}

