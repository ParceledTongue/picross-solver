import timeit
import constants as c
import possibilities as p

settings = {
    'print_possibility_calcs': False,
    'print_steps': False
}

def solve(hints):
    start = timeit.default_timer()

    width = len(hints['columns'])
    height = len(hints['rows'])

    steps = 0

    solved_rows = [False] * height
    solved_columns = [False] * width
    is_solved = False

    valid_lines = {}

    board = []
    for i in range(height):
        board.append([c.UNKNOWN] * width)
    
    for i in range(height):
        hint = hints['rows'][i]
        key = 'r' + str(i)
        valid_lines[key] = p.find_valid_lines(hint, width)
        if settings['print_possibility_calcs']:
            print('Calculated possibilities for ' + key)

    for j in range(width):
        hint = hints['columns'][j]
        key = 'c' + str(j)
        valid_lines[key] = p.find_valid_lines(hint, height)
        if settings['print_possibility_calcs']:
            print('Calculated possibilities for ' + key)
    
    exit = False
    while not exit:
        for i in range(height):
            if exit:
                break

            if solved_rows[i]:
                continue
            
            current_row = get_row_copy(board, i)
            key = 'r' + str(i)
            current_valid_lines = valid_lines[key]
            reduce_valid_lines(current_row, current_valid_lines)
            apply_row(board, find_definite_line(current_valid_lines), i)
            steps += 1

            if len(current_valid_lines) == 1:
                solved_rows[i] = True
            exit = is_solved_shortcut(solved_rows, solved_columns)

            if settings['print_steps']:
                print('Step ' + str(step) + ':')
                print_board(board)
                print

        for j in range(width):
            if exit:
                break

            if solved_columns[j]:
                continue

            current_column = get_column_copy(board, j)
            key = 'c' + str(j)
            current_valid_lines = valid_lines[key]
            reduce_valid_lines(current_column, current_valid_lines)
            apply_column(board, find_definite_line(current_valid_lines), j)
            steps += 1
            
            if len(current_valid_lines) == 1:
                solved_columns[j] = True
            exit = is_solved_shortcut(solved_rows, solved_columns)

            if settings['print_steps']:
                print_board(board)
                print

    stop = timeit.default_timer()

    print_board(board)
    print('Steps: ' + str(steps))
    print('Time: ' + str(stop - start) + ' sec')
    print
    return board

def is_solved_shortcut(solved_rows, solved_columns):
    for row in solved_rows:
        if not row:
            return False

    for column in solved_columns:
        if not column:
            return False

    return True

def get_row_copy(board, i):
    return board[i][:]

def get_column_copy(board, j):
    return [row[j] for row in board]

def apply_row(board, row, i):
    for j in range(len(row)):
        if board[i][j] == c.UNKNOWN:
            board[i][j] = row[j]

def apply_column(board, column, j):
    for i in range(len(board)):
        if board[i][j] == c.UNKNOWN:
            board[i][j] = column[i]

def reduce_valid_lines(known_line, valid_lines):
    to_remove = []
    for line in valid_lines:
        for i in range(len(known_line)):
            if not known_line[i] == c.UNKNOWN and not line[i] == known_line[i]:
                to_remove.append(line)
                break
    
    for line in to_remove:
        valid_lines.remove(line)

def find_definite_line(valid_lines):
    dim = len(valid_lines[0])
    definite_line = [c.UNKNOWN] * dim
    
    definite_filled = [True] * dim
    definite_empty = [True] * dim
    for line in valid_lines:
        for i in range(dim):
            definite_filled[i] = definite_filled[i] and line[i] == c.FILLED
            definite_empty[i] = definite_empty[i] and line[i] == c.EMPTY

    for i in range(dim):
        if definite_filled[i]: definite_line[i] = c.FILLED
        elif definite_empty[i]: definite_line[i] = c.EMPTY

    return definite_line

def print_board(board):
    for row in board:
        row_string = ''
        for cell in row:
            if cell == c.FILLED: row_string += 'X '
            elif cell == c.EMPTY: row_string += '- '
            else: row_string += '  '
        print(row_string)
