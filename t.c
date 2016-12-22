#include<omp.h>
#include<stdio.h>

// Written to figure out what the hell is going on with the HPCC

int main() {
	int i;
	double t0 = omp_get_wtime();
	for (i = 0; i < 100000000; i++) ;
	double t1 = omp_get_wtime();
	# pragma omp parallel for num_threads(2)
	for (i = 0; i < 100000000; i++) ;
	double t2 = omp_get_wtime();
	printf("%f, %f\n", t1 - t0, t2 - t1);
	return 0;
}
