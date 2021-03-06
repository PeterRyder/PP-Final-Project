// Final Project
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

/* array storage type */
#define ARRAY_TYPE unsigned int

/* numbers will be generated between 0 and 1000 */
#define MULTIPLIER 1000

/* if we are running in the blue gene environment */
#define bg_env 0

/* load the proper timer for the environment */
#if bg_env
#include <hwi/include/bqc/A2_inlines.h>
#else
#include <time.h>
#endif

/* size of the combined array */
unsigned int g_array_size = 0;

/* number of elements to sort per rank */
unsigned int g_ints_per_rank = 0;

int g_my_rank = -1;
int g_commsize = -1;

/* the combined array pointer */
ARRAY_TYPE *g_main_array = NULL;

/* timers for the proper environment */
#if bg_env
unsigned long long start_cycle_time = 0;
unsigned long long end_cycle_time = 0;
#else
clock_t start;
clock_t end;
#endif

/* generates a random array */
void generate_array(ARRAY_TYPE *array);

/* debug to print the array */
void print_array(ARRAY_TYPE* A, int size, int rank);

/* comparison function for sorting */
int compare (const void *a, const void *b);

/* sorting function */
void sort(ARRAY_TYPE *array);

/* merges two arrays together and sorts */
ARRAY_TYPE *merge(ARRAY_TYPE *A, ARRAY_TYPE *B, int a_size, int b_size); 

/* destroys the allocated memory */
void cleanup();

int main(int argc, char* argv[]) {

	if (argc != 2) {
		printf("Wrong arguments\n");
		printf("usage: assignment4 [matrix_size]\n");
		return -1;
	}

#if DEBUG
	printf("Got %d argument(s)\n", argc);

	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}
#endif

	/* the size of the combined array */
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
    ARRAY_TYPE *g_array = malloc(g_ints_per_rank * sizeof(ARRAY_TYPE));

#if DEBUG
    if (g_my_rank == 0)
    	printf("Ints per rank: %d\n", g_ints_per_rank);
#endif

#if DEBUG
    if (g_my_rank == 0)
		printf("Running simulation with %d ranks\n", g_commsize);
#endif

    /* start the timer */
    if (g_my_rank == 0) {
#if bg_env
    	start_cycle_time = GetTimeBase();
#else 
		start = clock();
#endif    	
    }

    /* initialize the array with rank 0 */
	if (g_my_rank == 0) {

		/* inialize array memory */	
		g_main_array = malloc( g_array_size * sizeof(ARRAY_TYPE));

		/* generate the main array */
		generate_array(g_main_array);
		for (int i = 0; i < g_commsize; i++) {
			MPI_Request status;

			/* find the index of the array to send */
			int starting_index = (g_array_size / g_commsize) * i;

			/* send piece to rank */
			MPI_Isend(&g_main_array[starting_index], g_ints_per_rank, 
				MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, &status);
			MPI_Request_free(&status);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Request request;
	MPI_Status status;
	
	MPI_Irecv(g_array, g_ints_per_rank, MPI_UNSIGNED, 
		0, 0, MPI_COMM_WORLD, &request);

	MPI_Wait(&request, &status);

	MPI_Barrier(MPI_COMM_WORLD);

	/* sort the piece of the array */
	qsort(g_array, g_ints_per_rank, sizeof *g_array, compare);

	int step = 1;
	int cur_size = g_ints_per_rank;
	while(step < g_commsize)
	{
		if(g_my_rank % (2 * step) == 0)
		{
			if(g_my_rank + step < g_commsize)
			{
				int recv_size = 0;
				MPI_Irecv(&recv_size, 1, MPI_INT, g_my_rank + step, 0, MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &status);
				ARRAY_TYPE *temp = (ARRAY_TYPE *)malloc(recv_size * sizeof(ARRAY_TYPE));
				MPI_Irecv(temp, recv_size, MPI_UNSIGNED, g_my_rank + step, 1, MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &status);
				g_array = merge(g_array, temp, cur_size, recv_size);
				cur_size = cur_size + recv_size;
			}
		}
		else
		{
			int send_loc = g_my_rank - step;
			MPI_Isend(&cur_size, 1, MPI_INT, send_loc, 0, MPI_COMM_WORLD, &request);
			MPI_Request_free(&request);
			MPI_Isend(g_array, cur_size, MPI_UNSIGNED, send_loc, 1, MPI_COMM_WORLD, &request);
			MPI_Request_free(&request);
			break;
		}
		step = step*2;
	}
	if(g_my_rank == 0)
	{
		g_main_array = g_array;
	}

	/* stop the timer */
	if (g_my_rank == 0) {
#if bg_env
    	end_cycle_time = GetTimeBase();
#else
    	end = clock();
#endif	
	}

    MPI_Barrier(MPI_COMM_WORLD);

    /* destroy the allocated memory */
    if (g_my_rank == 0) {
   		cleanup();
    }

    /* output the results */
	if (g_my_rank == 0) {
#if bg_env
	    printf("Completed in: %llu\n", end_cycle_time - start_cycle_time);
#else
		printf("Completed in: %Lf\n", (long double)(end - start));
#endif	
	}

	/* Finalize the MPI tasks */
 	MPI_Finalize();

	return 0;
}

/* generates a random unsorted array */
void generate_array(ARRAY_TYPE *array) {
	for (unsigned int i = 0; i < g_array_size; i++) {
		array[i] = GenVal(g_my_rank) * MULTIPLIER;
	}
}

/* compare function for sorting */
int compare (const void *a, const void *b) {
  return (*(int*)a - *(int*)b);
}

/* destroys the array */
void cleanup() {
	//free(g_array);
	free(g_main_array);
}

/* Merge two given arrays */
ARRAY_TYPE *merge(ARRAY_TYPE *A, ARRAY_TYPE *B, int a_size, int b_size) 
{
	int a_iter = 0; 
	int b_iter = 0;
	int c_iter = 0;
        int c_size = a_size + b_size;

	// Create output array that is the size of both inputs
	ARRAY_TYPE *C = (ARRAY_TYPE *)malloc(c_size * sizeof(ARRAY_TYPE));
        while( (a_iter < a_size) && (b_iter < b_size))
	{
		// if next element in A is smaller insert it
		if(A[a_iter] <= B[b_iter])
		{
			C[c_iter] = A[a_iter];
 //       printf("\ninsert A[%d] = %d into C[%d] = %d\n", a_iter, A[a_iter], c_iter, C[c_iter]);
			a_iter++;
			c_iter++;
		}
		// otherwise the element in B is lower so insert it
		else
		{
			C[c_iter] = B[b_iter];
//        printf("\ninsert B[%d] = %d into C[%d] = %d\n", b_iter, B[b_iter], c_iter, C[c_iter]);
			b_iter++;
			c_iter++;
		}
	}
	// Add the rest of the elements in the unfinished list
	if (a_iter >= a_size)
	{
		for (; c_iter < c_size; b_iter++)
		{
			C[c_iter] = B[b_iter];
			c_iter++;
		}
	}
	else if (b_iter >= b_size)
	{
		for (; c_iter < c_size; a_iter++)
		{
			C[c_iter] = A[a_iter];
			c_iter++;
		}
	}

	//theoretically free A and B?
	//free(A);
	//free(B);

	return C;
}

void print_array(ARRAY_TYPE* A, int size, int rank)
{
	printf("rank %d: ", rank);
	for (int i = 0; i < size; i++) {
		printf("%u,", A[i]);
	}
	printf("\n");
}


