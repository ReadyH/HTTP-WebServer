#ifndef TIMER_H
#define TIMER_H

#include "heap.h"
#include "http.h"

#define TIMEOUT 1000
#define PQ_SIZE 10

// 处理超时节点的函数指针
typedef int (*timeoutHandler)(httpRequest* request);
typedef struct Heap* PriorityQueue;

// 定时器节点
struct timer {
    size_t key;
    int deleted;
    timeoutHandler handler;
    httpRequest* request;
};

extern size_t currentMsec;
extern struct Heap theTimer;
int createTimer(int pqSize);
void updateCurrentTime();
int getTime();
void addTimerNode(httpRequest* request, size_t timeout, timeoutHandler handler);
void deleteTimerNode(httpRequest* request);
void handleExpireTimerNode();

#endif /TIMER_H
