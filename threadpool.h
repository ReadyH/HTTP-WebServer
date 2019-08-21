#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define IMMEDIATE       1
#define GRACEFUL        2

#define TP_NULL         -1
#define TP_THREADS_NULL -2
#define TP_TASK_NULL    -3
#define TP_LOCK_FAIL    -4
#define TP_COND_FAIL    -5
#define TP_THREAD_CREATE_FAIL -6
#define TP_THREAD_RELEASE_FAIL -7
#define TP_ALREADY_SHUTDOWN   -8

struct task {
    void (*func)(void*);
    void* args;
    struct task* next;
};

struct threadPool {
    int started;
    int shutdown;
    int taskSize;
    int threadCount;
    struct task* taskHead;
    pthread_t* threads;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

struct threadPool* createThreadPool(int threadNum);
int addThreadPool(struct threadPool* tp, void (*func)(void*), void* args);
int releaseThreads(struct threadPool* tp, int howToShutdown);
int freeThreadPool(struct threadPool* tp);


void printErrType(char* errmsg);

#endif THREADPOOL_H
