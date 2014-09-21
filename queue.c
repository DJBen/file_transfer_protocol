
/*	queue.c

	Implementation of a FIFO queue abstract data type.

	by: Steven Skiena
	begun: March 27, 2002
*/


/*
Copyright 2003 by Steven S. Skiena; all rights reserved.

Permission is granted for use in non-commerical applications
provided this copyright notice remains intact and unchanged.

This program appears in my book:

"Programming Challenges: The Programming Contest Training Manual"
by Steven Skiena and Miguel Revilla, Springer-Verlag, New York 2003.

See our website www.programming-challenges.com for additional information.

This book can be ordered from Amazon.com at

http://www.amazon.com/exec/obidos/ASIN/0387001638/thealgorithmrepo/

*/


#include "queue.h"

void init_queue(queue *q)
{
        q->first = 0;
        q->last = QUEUESIZE-1;
        q->count = 0;
}

void enqueue(queue *q, int x)
{
        if (q->count >= QUEUESIZE) {
	       printf("Warning: queue overflow enqueue x=%d\n",x);
        } else {
                q->last = (q->last+1) % QUEUESIZE;
                q->q[ q->last ] = x;
                q->count = q->count + 1;
        }
}

int dequeue(queue *q)
{
        int x = 0;
        if (q->count <= 0) {
                printf("Warning: empty queue dequeue.\n");
        } else {
                x = q->q[ q->first ];
                q->first = (q->first+1) % QUEUESIZE;
                q->count = q->count - 1;
        }

        return(x);
}

bool empty(queue *q)
{
        if (q->count <= 0) {
                return TRUE;
        } else {
                return FALSE;
        }
}

void print_queue(queue *q)
{
        int i;
        if (q->count == 0) return;
        printf("queue (%d elements): ", q->count);
        i=q->first;
        while (i != q->last) {
                printf("%d ",q->q[i]);
                i = (i+1) % QUEUESIZE;
        }
        printf("%d ",q->q[i]);
        printf("\n");
}

int queueContains(queue *q, int element) {
        int i;
        if (q->count == 0) return FALSE;
        i = q->first;
        while (i != q->last) {
                if (q->q[i] == element) {
                        return TRUE;
                }
                i = (i + 1) % QUEUESIZE;
        }
        if (q->q[i] == element) return TRUE;
        return FALSE;
}

