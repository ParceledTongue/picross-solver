NONOGRAM SOLVER [BETA]
======================

This beta version is still in its Python prototype, with the intent that as
soon as the algorithm is satisfactory it will be ported to C and parallelized.
I am not sure how this falls in the context of the beta requirements. It is
more functional than necessary (it can solve the three examples shown in 
`output.txt`, for instance). It also includes a robust outline of the algorithm
and its requisite structures, which will not change a great deal when ported to
C. The reason I have not started to port yet, however, is that the algorithm
still fails on certain solveable puzzles, and it uses an unreasonable amount of
memory for a typical 50x50 puzzle (which, granted, is very large for a
nonogram). I may need to rethink fundamental aspects of the algorithm before
porting to C.

Major Files
-----------

### solver.py

This is the primary module of the prototype. `solve` is called on a set of
"hints" (see `examples.py`) and the board solution is returned as a 2D array,
with `0` representing an unfilled square and `1` representing a filled one.

### possibilities.py

This helper module exists to perform a fundamental atomic calculation in the
algorithm, which is to take in a single-line hint and the dimension of that
line (i.e. its length) and output an array of arrays, where each subarray
represents a line satisfying the hint. As above, `0` indicates an unfilled
square and `1` indicates a filled one.

### examples.py

This module contains member dicts, each of which represents a puzzle. Each dict
has an array of row hints and an array of column hints; these dicts are what
is passed into `solve` (see `solver.py`) as "hints".

### run.py

This is just a test script to automate running the solver on certain puzzles.
