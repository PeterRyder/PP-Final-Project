all: clcg4.h clcg4.c main.c
	gcc -I. -Wall -O3 -c clcg4.c -o clcg4.o
	mpicc -I. -W -Wall -D DEBUG -std=c99 -O3 main.c clcg4.o -lm -o out
