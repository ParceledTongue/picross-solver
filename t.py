FILLED = 1
EMPTY = 2

def gen_row(size, hint):
    """Create all patterns of a row or col that match given runs."""
    def gen_seg(runs, spaces):
        if not runs:
            return [[EMPTY] * spaces]
        return [[EMPTY] * x + runs[0] + tail
                for x in xrange(1, spaces - len(runs) + 2)
                for tail in gen_seg(runs[1:], spaces - x)]
    runs = [[FILLED] * i for i in hint]
    print runs
    seg = gen_seg(runs, size + 1 - sum(hint))
    return [x[1:] for x in seg]
