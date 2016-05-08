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
#else
#include <time.h>
#endif

unsigned int g_array_size = 0;
unsigned int g_ints_per_rank = 0;

int g_my_rank = -1;
int g_commsize = -1;

//ARRAY_TYPE *g_array = NULL;
ARRAY_TYPE *g_main_array = NULL;

#if bg_env
unsigned long long start_cycle_time = 0;
unsigned long long end_cycle_time = 0;
#else
clock_t start;
clock_t end;
#endif

void generate_array(ARRAY_TYPE **array);
void print_array(ARRAY_TYPE* A, int size, int rank);
int compare (const void *a, const void *b);
void sort(ARRAY_TYPE *array);
ARRAY_TYPE *merge(ARRAY_TYPE *A, ARRAY_TYPE *B, int a_size, int b_size); 
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
		generate_array(&g_main_array);
		for (int i = 0; i < g_commsize; i++) {
			MPI_Request status;

			int starting_index = (g_array_size / g_commsize) * i;
			//printf("Starting index: %d\n", starting_index);
			//printf("Sending to: %d\n", i);

			MPI_Isend(&g_main_array[starting_index], g_ints_per_rank, 
				MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &status);
			MPI_Request_free(&status);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Request request;
	MPI_Status status;
	
	MPI_Irecv(g_array, g_ints_per_rank, MPI_UNSIGNED, 
		0, 1, MPI_COMM_WORLD, &request);

	MPI_Wait(&request, &status);

#if DEBUG
	print_array(g_array, g_ints_per_rank, g_my_rank);
#endif

	//printf("%d Got receive\n", g_my_rank);

	MPI_Barrier(MPI_COMM_WORLD);

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

	/* pass arrays back to rank 0 */
/*	MPI_Isend(g_array, g_ints_per_rank, MPI_UNSIGNED, 0, 2, MPI_COMM_WORLD, &request);
	MPI_Request_free(&request);
	MPI_Barrier(MPI_COMM_WORLD);
	if (g_my_rank == 0)
	{
		ARRAY_TYPE ** recieved_arrays = malloc(g_commsize * sizeof(ARRAY_TYPE*));
		for(int i = 0; i < g_commsize; i++)
		{
			recieved_arrays[i] = malloc(g_ints_per_rank * sizeof(ARRAY_TYPE));
			MPI_Irecv(recieved_arrays[i], g_ints_per_rank, MPI_UNSIGNED, i, 2, MPI_COMM_WORLD, &receive);
		}
		int cur_size = g_ints_per_rank;
		//free(g_main_array);
		g_main_array = recieved_arrays[0];
		for(int i = 1; i < g_commsize; i++)
		{
			g_main_array = merge(g_main_array, recieved_arrays[1], cur_size, g_ints_per_rank);
			cur_size += g_ints_per_rank;
		}
	}
*/

	if (g_my_rank == 0) {
#if bg_env
    	end_cycle_time = GetTimeBase();
#else
    	end = clock();
#endif	
	}


#if DEBUG
	print_array(g_array, g_ints_per_rank, g_my_rank);
#endif

#if DEBUG
    	MPI_Barrier(MPI_COMM_WORLD);
	if(g_my_rank == 0){
		printf("\n\nFinal sorted Array:\n");
		print_array(g_main_array, g_array_size, g_my_rank);
		printf("\n\n");
	}
#endif

    MPI_Barrier(MPI_COMM_WORLD);
    //free(g_array);
    if (g_my_rank == 0) {
   		cleanup();
    }

	if (g_my_rank == 0) {
#if bg_env
	    printf("Completed in: %llu\n", end_cycle_time - start_cycle_time);
#else
		clock_t diff = end - start;
		int msec = diff * 1000 / CLOCKS_PER_SEC;
		printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);
#endif	
	}


 	MPI_Finalize();

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

