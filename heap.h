#ifndef HEAP_H
#define HEAP_H

#include "timer.h"

typedef struct timer* T;

struct Heap {
    int capacity;
    int size;
    T* elements;
};

typedef struct Heap* PriorityQueue;

int initialize(PriorityQueue pq, int maxSize);
void destroy(PriorityQueue pq);
void makeEmpty(PriorityQueue pq);
void insert(T t, PriorityQueue pq);
T deleteMin(PriorityQueue pq);
T findMin(PriorityQueue pq);
int isEmpty(PriorityQueue pq);
int isFull(PriorityQueue pq);

int compare(T t1, T t2);

#endif // HEAP_H
