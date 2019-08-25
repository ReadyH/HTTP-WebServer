// 堆：参考《数据结构与算法分析》 Mark Allen Weiss著

#include "heap.h"
#include "timer.h"
#include <stdlib.h>
#include "utils.h"

#define MIN_PQ_SIZE 10

// 初始化堆
int initialize(PriorityQueue pq, int maxSize) {

    if(maxSize < MIN_PQ_SIZE)
        maxSize = MIN_PQ_SIZE;
    // pq = (struct Heap*) malloc(sizeof(struct Heap));
    //if(pq == NULL)
        //errorHandler("Out of space");
    pq->elements = (struct timer*)malloc((maxSize + 1) * sizeof(struct timer));
    if(pq->elements == NULL)
        errorHandler("Out of space");

    pq->capacity = maxSize;
    pq->size = 0;
    pq->elements[0] = NULL;

    return 0;
}

// 清空堆
void makeEmpty(PriorityQueue pq) {
    pq->size = 0;
}

// 定时器节点比较函数
int compare(T t1, T t2) {
    if(t2 == NULL) {
        return 0;
    }
    return (t1->key < t2->key) ? 1 : 0;
}

// 堆插入
void insert(T t, PriorityQueue pq) {
    int i = 0;
    if(isFull(pq))
        errorHandler("Priority queue is full");
    // for(i = ++pq->size; compare(pq->elements[i / 2], t); i /= 2)
    for(i = ++pq->size; compare(t, pq->elements[i / 2]); i /= 2)
        pq->elements[i] = pq->elements[i / 2];
    pq->elements[i] = t;
}

// 删除最小节点（小顶堆）
T deleteMin(PriorityQueue pq) {
    int i = 0, child = 0;
    if(isEmpty(pq))
        errorHandler("Priority queue is empty");
    T minElement = pq->elements[1];
    T lastElement = pq->elements[pq->size--];
    for(i = 1; i * 2 <= pq->size; i = child) {
        child = i * 2;
        if(child != pq->size && compare(pq->elements[child + 1], pq->elements[child]))
            child++;
        if(compare(pq->elements[child], lastElement))
            pq->elements[i] = pq->elements[child];
        else
            break;
    }
    pq->elements[i] = lastElement;
    return minElement;
}

// 返回最小节点
T findMin(PriorityQueue pq) {
    if(!isEmpty(pq))
        return pq->elements[1];
    errorHandler("Priority Queue is empty");
    return pq->elements[0];
}

// 判空
int isEmpty(PriorityQueue pq) {
    return pq->size == 0;
}

// 判满
int isFull(PriorityQueue pq) {
    return pq->size == pq->capacity;
}

// 销毁堆
void destroy(PriorityQueue pq) {
    free(pq->elements);
    free(pq);
}
