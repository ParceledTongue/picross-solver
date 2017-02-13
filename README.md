NONOGRAM SOLVER
===============

This program solves Nonogram/Picross/Hanjie puzzles using a parallelized
approach and memory-efficient data structures. Use `make` to compile, and run
it via `./bin/solver PATH_TO_INPUT_FILE`.


Input
-----

The first line must contain two integers: the number of rows followed by the
number of columns. The lines that follow are the row hints and column hints,
in that order (and from top to bottom / left to right). Any lines beginning
with non-numeric characters, including newlines, are ignored (so you can use
them for formatting). 

For example, this puzzle

              1
            1 1 1
          5 1 1 3

      4   # # # #
      1   # . . .
    1 2   # . # #
    1 1   # . . #
      4   # # # #

can be represented by this file:

```
5 4

---

4
1
1 2
1 1
4

---

5
2
1 1 1
3

The line breaks and dashes are ignored. So is this!
```


Files
-----

For now, the entire implementation lives in `src/solver.c` (I know, I know... 
Sorry!). Example input files are in the `puzzles` folder. The `slurm-output`
folder contains some example output from executing `test.slurm` on the HPCC
using different numbers of cores. Finally, the old Python beta is in `proto`.


Major Data Structures
---------------------

All major data structures have their own sections where all the functions that
operate on them live.

### `Line`
This is the most interesting structure. It is effectively an array of bits, and
it has many bitwise operations implemented on it (shift, and, or). Since there
are so many `Line`s involved in every execution of the algorithm, having such a
time- and space-efficient implementation is incredibly helpful.

### `LineNode`, `LineList`
Simple (and partial) doublely linked list implementation for stringing together
`Line`s. Actually, it was silly to make the `LineList` structure; I should
really just be passing around head nodes. 

### `Board`
Holds two lists of `LineList`s, one for the rows and one for the columns. Each
row and each column has its own `LineList` which holds every potential line 
configuration for that row or column at each step of the algorithm.

### `Hint`
Represents a hint for a single row or column, e.g. `6 3 2`.

### `Hints`
Somewhat confusingly named, but this just holds all the hints for a given
puzzle. Two lists of hints, one for row hints and one for column hints.


Future Goals
------------

- First and foremost, this is badly in need of refactoring (it's implemented in 
  a single 500-line file). It's obviously the work of a person who as never 
  programmed in C before working through mounting exhaustion. I'd like to 
  address this first before tackling any other goals.

- I want to implement certain elements of the Python implemenation
  [here](rosettacode.org/wiki/Nonogram_solver#Python), namely:
    * The ability to solve puzzles not tractable via the reduction strategy
      employed now.
    * The ability to determine whether a puzzle's solution is unique.
    * Other optimizations (some of which are present in the Python prototype I
      designed independently but never made it into the final implementation).

- The parallelization, while functional and useful, is not thoughtfully
  implemented. I'm sure there is more to be done on this front.

- Once the design is sane, this needs a test routine and error checking for all
  its constituent parts. 
