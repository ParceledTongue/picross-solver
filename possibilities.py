import constants as c

# Take in a single-line hint and the line's dimension, and return an array
# containing all valid lines for that hint-dimension pair.
def find_valid_lines(hint, dim):
    if hint == [0]:
        return [[c.EMPTY] * dim]

    valid_lines = []
    
    arrangement = {}

    arrangement['sizes'] = hint[:]
    arrangement['offsets'] = [0] * len(hint)

    offset_sum_max = dim - sum(hint) - (len(hint) - 1)

    exit = False
    while not exit:
        valid_lines.append(arrangement_to_line(arrangement, dim))
        arrangement['offsets'][-1] += 1
        place = -1
        while sum(arrangement['offsets']) > offset_sum_max:
            arrangement['offsets'][place] = 0
            place -= 1
            if abs(place) <= len(arrangement['offsets']):
                arrangement['offsets'][place] += 1
            else:
                exit = True

    return valid_lines

# Take in an "arrangement" (as used in find_valid_lines) and convert it to the
# standard array representation for the corresponding line.
def arrangement_to_line(arrangement, dim):
    line = [c.EMPTY] * dim
    pos = 0
    for i in range(len(arrangement['sizes'])):
        size = arrangement['sizes'][i]
        offset = arrangement['offsets'][i]

        pos += offset
        for j in range(size):
            line[pos + j] = c.FILLED
        pos += size + 1

    return line
