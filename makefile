picrossmake: src/solver.c
	mkdir -p bin
	gcc -o bin/solver src/solver.c -fopenmp
