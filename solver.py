import operator
import timeit
import sys
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

    valid_lines = {}
    valid_lines_bank = {} # for memoization

    board = []
    for i in range(height):
        board.append([c.UNKNOWN] * width)
    
    # Calculate possible lines for all rows
    for i in range(height):
        hint = hints['rows'][i]
        key = 'r' + str(i)
        bank_key = str([hint, width])
        if not bank_key in valid_lines_bank:
            valid_lines_bank[bank_key] = p.find_valid_lines(hint, width)
        valid_lines[key] = valid_lines_bank[bank_key][:]
        if settings['print_possibility_calcs']:
            print('Calculated possibilities for ' + key)

    # Calculate possible lines for all columns
    for j in range(width):
        hint = hints['columns'][j]
        key = 'c' + str(j)
        bank_key = str([hint, height])
        if not bank_key in valid_lines_bank:
            valid_lines_bank[bank_key] = p.find_valid_lines(hint, height)
        valid_lines[key] = valid_lines_bank[bank_key][:]
        if settings['print_possibility_calcs']:
            print('Calculated possibilities for ' + key)

    considering = valid_lines.keys()
    
    while not is_solved(solved_rows, solved_columns):
        if not considering:
            print('Giving up :(')
            return None
        considering = sorted(considering, key = lambda x: len(valid_lines[x]))

        for key in considering:
            steps += 1

            if settings['print_steps']:
                print('Step ' + str(steps))
                print('  Examining ' + key)
                print('  ' + str(len(valid_lines[key])) + ' valid lines')
                print('  Queue is ' + str(considering[:5]) + '...')
            
            if key[0] == 'r':
                getter = get_row_copy
                setter = apply_row
                tracker = solved_rows
                other_tracker = solved_columns
                to_consider_pre = 'c'
            else:
                getter = get_column_copy
                setter = apply_column
                tracker = solved_columns
                other_tracker = solved_rows
                to_consider_pre = 'r'
            index = int(key[1:])

            current_line = getter(board, index)
            current_valid_lines = valid_lines[key]
            reduce_valid_lines(current_line, current_valid_lines)
            changed = setter(board, find_definite_line(current_valid_lines), index)
            
            if changed:
                update_solved(board, solved_rows, solved_columns)
            for i in changed:
                to_consider_key = to_consider_pre + str(i)
                if to_consider_key not in considering and not other_tracker[i]:
                    considering.append(to_consider_key)

            considering.remove(key)

            if len(current_valid_lines) == 1:
                tracker[index] = True

            if settings['print_steps']:
                print_board(board)
                print
            if changed:
                break

    stop = timeit.default_timer()

    print_board(board)
    print('Steps: ' + str(steps))
    print('Time: ' + str(stop - start) + ' sec')
    print
    return board

# Modify solved_rows and solved_columns to accurately reflect the state of the
# board passed in. This is kind of an ugly mix of functional and data-driven
# style and should probably be refactored.
def update_solved(board, solved_rows, solved_columns):
    for i in range(len(solved_rows)):
        if not solved_rows[i]:
            solved = True
            for cell in get_row_copy(board, i):
                if cell == c.UNKNOWN:
                    solved = False
                    break
            if solved: solved_rows[i] = True

    for j in range(len(solved_columns)):
        if not solved_columns[i]:
            solved = True
            for cell in get_column_copy(board, j):
                if cell == c.UNKNOWN:
                    solved = False
                    break
            if solved: solved_columns[j] = True

def is_solved(solved_rows, solved_columns):
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

# Apply functions return list of changed indices

def apply_row(board, row, i):
    changed = []
    for j in range(len(row)):
        if board[i][j] == c.UNKNOWN and not row[j] == c.UNKNOWN:
            board[i][j] = row[j]
            changed.append(j)
    return changed

def apply_column(board, column, j):
    changed = []
    for i in range(len(board)):
        if board[i][j] == c.UNKNOWN and not column[i] == c.UNKNOWN:
            board[i][j] = column[i]
            changed.append(i)
    return changed

def reduce_valid_lines(known_line, valid_lines): 
    valid_lines[:] = [line for line in valid_lines if is_valid(known_line, line)]

def is_valid(known_line, line):
    for i in range(len(known_line)):
        if not known_line[i] == c.UNKNOWN and not line[i] == known_line[i]:
            return False
    return True

# Return all the knowledge we have of a line based on the current set of valid
# lines.
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
