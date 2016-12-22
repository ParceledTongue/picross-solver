#include<time.h>
#include<stdio.h>

// Written to figure out what the hell is going on with the HPCC

int main() {
	int i;
	time_t t0 = clock();
	for (i = 0; i < 100000000; i++) ;
	time_t t1 = clock();
	# pragma omp parallel for num_threads(2)
	for (i = 0; i < 100000000; i++) ;
	time_t t2 = clock();
	printf("%d, %d\n", t1 - t0, t2 - t1);
	return 0;
}
