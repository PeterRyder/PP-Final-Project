// Assignment 4/5
// Peter Ryder, Brian Kovacik, Matt Holmes

/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#include "clcg4.h"
#include <mpi.h>

int g_array_size = 0;

int g_my_rank = -1;
int g_commsize = -1;


int main(int argc, char* argv[]) {

	if (argc != 2) {
		printf("Wrong arguments\n");
		printf("usage: assignment4 [matrix_size]");
		return -1;
	}

#if DEBUG
	printf("Got %d argument(s)\n", argc);

	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}
#endif

	g_array_size = atoi(argv[1]);

	if (g_array_size <= 0) {
		printf("Bad arguments\n");
		return(-1);
	}

	MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &g_commsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_my_rank);

    g_ints_per_rank = g_array_size / g_commsize;

#if DEBUG
    if (g_my_rank == 0)
		printf("Running simulation with %d ranks and %d threads per rank\n", g_ranks, g_threads_per_rank);
#endif

	InitDefault();

#if DEBUG
    printf("Rank %d of %d has been started and a first Random Value of %lf\n", 
        g_my_rank, g_commsize, GenVal(g_my_rank));
#endif

    MPI_Barrier(MPI_COMM_WORLD);

#if DEBUG
    if (g_my_rank == 0)
    	printf("Ints per rank: %d\n", g_ints_per_rank);
#endif


	return 0;
}