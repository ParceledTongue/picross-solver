#include <stdio.h>
#include <stdlib.h>

int PUZZLE_HEIGHT = 10;
int PUZZLE_WIDTH = 10;

typedef enum {FILLED, EMPTY, UNKNOWN} Cell;

typedef struct {
    int size;
    Cell *cells;
} Line;

typedef struct {
    int numRows;
    int numCols;
    Line *rows;
    Line *cols;
} Board;

Line makeInitialLine(int size) {
    Line line;
    line.size = size;
    line.cells = malloc(line.size * sizeof(Cell));
    
    int i;
    for (i = 0; i < line.size; i++) {
        line.cells[i] = UNKNOWN;
    }

   return line; 
}

void printLine(Line line) {
    int i;
    for (i = 0; i < line.size; i++) {
        if (line.cells[i] == FILLED)
            printf("#");
        if (line.cells[i] == EMPTY)
            printf(".");
        if (line.cells[i] == UNKNOWN)
            printf("?");
    }
    printf("\n");
}

void printBoard(Board board) {
    int i;
    for (i = 0; i < board.numRows; i++) {
        printLine(board.rows[i]);
    }
    printf("\n");
}

int main() {
    Board board;
    board.numRows = PUZZLE_HEIGHT;
    board.numCols = PUZZLE_WIDTH;
    board.rows = malloc(board.numRows * sizeof(Line));
    board.cols = malloc(board.numCols * sizeof(Line));

    int i, j;
    // initialize rows
    for (i = 0; i < board.numRows; i++) {
        board.rows[i] = makeInitialLine(board.numCols);
    }
    // initialize columns
    for (j = 0; j < board.numCols; j++) {
        board.cols[j] = makeInitialLine(board.numRows);
    }

    printBoard(board);

    return 0;
}


