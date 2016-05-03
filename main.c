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

#define ARRAY_TYPE unsigned int
#define MULTIPLIER 1000
#define bg_env 0

#if bg_env
#include <hwi/include/bqc/A2_inlines.h>
#endif

unsigned int g_array_size = 0;
unsigned int g_ints_per_rank = 0;

int g_my_rank = -1;
int g_commsize = -1;

ARRAY_TYPE *g_array = NULL;
ARRAY_TYPE *g_main_array = NULL;

unsigned long long start_cycle_time = 0;
unsigned long long end_cycle_time = 0;

void generate_array(ARRAY_TYPE **array);
int compare (const void *a, const void *b);
void sort(ARRAY_TYPE *array);
void cleanup();

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

	/* initialize the rng */
	InitDefault();

	/* initialize MPI */
	MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &g_commsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_my_rank);

    /* how many ints each rank is responsible for */
    g_ints_per_rank = g_array_size / g_commsize;

    g_array = calloc(g_ints_per_rank, sizeof(ARRAY_TYPE));

#if DEBUG
    if (g_my_rank == 0)
    	printf("Ints per rank: %d\n", g_ints_per_rank);
#endif

#if DEBUG
    if (g_my_rank == 0)
		printf("Running simulation with %d ranks\n", g_commsize);
#endif

#if DEBUG
    printf("Rank %d of %d has been started and a first Random Value of %lf\n", 
        g_my_rank, g_commsize, GenVal(g_my_rank));
#endif

    /* start the timer */
#if bg_env
    if (g_my_rank == 0)
    	start_cycle_time = GetTimeBase();
#endif

    /* initialize the array with rank 0 */
	if (g_my_rank == 0) {		
		generate_array(&g_main_array);

		for (unsigned int i = 0; i < g_array_size; i++) {
			printf("%d,", g_main_array[i]);
		}
		printf("\n");

		for (int i = 0; i < g_commsize; i++) {
			MPI_Request status;

			int starting_index = (g_array_size / g_commsize) * i;
			printf("Starting index: %d\n", starting_index);
			printf("Sending to: %d\n", i);

			MPI_Isend(&g_main_array[starting_index], g_ints_per_rank, 
				MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &status);
		}
	}

	MPI_Request receive;
	MPI_Status status;
	
	MPI_Irecv(g_array, g_ints_per_rank, MPI_UNSIGNED, 
		0, 1, MPI_COMM_WORLD, &receive);

	MPI_Wait(&receive, &status);

	printf("%d Got receive\n", g_my_rank);

	MPI_Barrier(MPI_COMM_WORLD);

	qsort(g_array, g_ints_per_rank, sizeof(ARRAY_TYPE), compare);

	/* pass arrays back to rank 0 */

#if bg_env
	if (g_my_rank == 0)
    	end_cycle_time = GetTimeBase();
#endif

#if DEBUG
	for (unsigned int i = 0; i < g_ints_per_rank; i++) {
		printf("%u,", g_array[i]);
	}
	printf("\n");
#endif

    MPI_Barrier(MPI_COMM_WORLD);

    if (g_my_rank == 0) {
   		cleanup();
    }

#if bg_env
    printf("Completed in: %llu\n", end_cycle_time - start_cycle_time);
#endif

 	MPI_Finalize();

 	printf("test\n");

	return 0;
}

/* generates a random unsorted array */
void generate_array(ARRAY_TYPE **array) {
	*array = calloc(g_ints_per_rank, sizeof(ARRAY_TYPE));

	for (unsigned int i = 0; i < g_array_size; i++) {
		(*array)[i] = GenVal(g_my_rank) * MULTIPLIER;
	}
}

int compare (const void *a, const void *b) {
  return (*(int*)a - *(int*)b);
}

/* destroys the array */
void cleanup() {
	free(g_array);
	//free(g_main_array);
}