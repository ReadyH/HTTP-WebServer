#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>

static void* worker(void* args);

// 初始化线程池
struct threadPool* createThreadPool(int threadNum) {
    struct threadPool* tp;
    int errType = 0;
    do {
        tp = (struct threadPool*)malloc(sizeof(struct threadPool));
        if(tp == NULL) {
            errType = TP_NULL;
            break;
        }
        tp->started = 0;
        tp->shutdown = 0;
        tp->taskSize = 0;
        tp->threadCount = 0;
        tp->threads = (pthread_t*)malloc(sizeof(pthread_t) * threadNum);
        if(tp->threads == NULL) {
            errType = TP_THREADS_NULL;
            break;
        }
        tp->taskHead = (struct task*)malloc(sizeof(struct task));
        if(tp->taskHead == NULL) {
            errType = TP_TASK_NULL;
            break;
        }
        tp->taskHead->func = NULL;
        tp->taskHead->args = NULL;
        tp->taskHead->next = NULL;

        if(pthread_mutex_init(&(tp->mutex), NULL) != 0) {
            errType = TP_LOCK_FAIL;
            break;
        }

        if(pthread_cond_init(&(tp->cond), NULL) != 0) {
            errType = TP_COND_FAIL;
            break;
        }

        for(int i = 0; i < threadNum; ++i) {
            if(pthread_create(&(tp->threads[i]), NULL, worker, (void*)tp) != 0) {
                errType = TP_THREAD_CREATE_FAIL;
                releaseThreads(tp, 0);
                break;
            }
            tp->threadCount++;
            tp->started++;
        }
    } while(0);

    if(errType != 0) {
        if(errType == TP_NULL) {
            printErrType("no space to create thread pool!");
        } else if(errType == TP_TASK_NULL) {
            printErrType("no space to create task list!");
        } else if(errType == TP_THREADS_NULL) {
            printErrType("no space to create threads!");
        } else if(errType == TP_LOCK_FAIL) {
            printErrType("create mutex failed!");
        } else if(errType == TP_COND_FAIL) {
            printErrType("create cond failed!");
        } else if(errType == TP_THREAD_CREATE_FAIL) {
            printErrType("failed in create one of threads!");
        } else {
            printErrType("unknown fail!");
        }
        if(tp != NULL) {
            freeThreadPool(tp);
        }
        return NULL;
    }
    return tp;
}

// 添加事件
int addThreadPool(struct threadPool* tp, void (*func)(void*), void* args) {

    int errType = 0, locked = 0;
    do {
        if(tp == NULL || func == NULL) {
            errType = TP_NULL;
            break;
        }
        if(pthread_mutex_lock(&(tp->mutex)) != 0) {
            errType = TP_LOCK_FAIL;
            break;
        }
        locked = 1;
        if(tp->shutdown) {
            errType = TP_ALREADY_SHUTDOWN;
            break;
        }

        struct task* t = (struct task*)malloc(sizeof(struct task));
        if(t == NULL) {
            errType = TP_TASK_NULL;
            break;
        }
        
        // 添加具体事件到任务链表
        t->func = func;
        t->args = args;
        
        t->next = tp->taskHead->next;
        tp->taskHead->next = t;
        tp->taskSize++;
        // 唤醒等待中的线程
        if(pthread_cond_signal(&(tp->cond)) != 0) {
            errType = TP_COND_FAIL;
            break;
        }
    }while(0);
    if(locked == 1) {
        pthread_mutex_unlock(&(tp->mutex));
    }
    if(errType != 0) {
        if(errType == TP_NULL) {
            printErrType("no space to create thread pool!");
        } else if(errType == TP_TASK_NULL) {
            printErrType("no space to create task list!");
        } else if(errType == TP_THREADS_NULL) {
            printErrType("no space to create threads!");
        } else if(errType == TP_LOCK_FAIL) {
            printErrType("create mutex failed!");
        } else if(errType == TP_COND_FAIL) {
            printErrType("create cond failed!");
        } else if(errType == TP_THREAD_CREATE_FAIL) {
            printErrType("failed in create one of threads!");
        } else {
            printErrType("unknown fail!");
        }
    }

    return errType;
}

// 释放线程池
int releaseThreads(struct threadPool* tp, int howToShutdown){
    int errType = 0, locked = 0;
    do {
        if(tp == NULL) {
            errType = TP_NULL;
            break;
        }
        if(pthread_mutex_lock(&(tp->mutex)) != 0) {
            errType = TP_LOCK_FAIL;
            break;
        }
        locked = 1;

        if(tp->shutdown) {
            errType = TP_ALREADY_SHUTDOWN;
            break;
        }

        tp->shutdown = (howToShutdown) ? GRACEFUL : IMMEDIATE;

        if(pthread_cond_broadcast(&(tp->cond)) != 0) {
            errType = TP_COND_FAIL;
            break;
        }

        if(pthread_mutex_unlock(&(tp->mutex)) != 0) {
            errType = TP_LOCK_FAIL;
            break;
        }
        locked = 0;

        for(int i = 0; i < tp->threadCount; ++i) {
            if(pthread_join(tp->threads[i], NULL) != 0) {
                errType = TP_THREAD_RELEASE_FAIL;
            }
        }
    } while(0);

    if(errType == 0) {
        pthread_mutex_destroy(&(tp->mutex));
        pthread_cond_destroy(&(tp->cond));
        freeThreadPool(tp);
    }

    return errType;
}

// 销毁线程池
int freeThreadPool(struct threadPool* tp){
    if(tp == NULL || tp->started > 0)
        return -1;

    if(tp->threads)
        free(tp->threads);

    struct task* oldTask;
    while(tp->taskHead->next){
        oldTask = tp->taskHead->next;
        tp->taskHead->next = tp->taskHead->next->next;
        free(oldTask);
    }
    return 0;
}


// 工作函数
void* worker(void* args){
    if(args == NULL) {
        return NULL;
    }

    struct threadPool* tp = (struct threadPool*) args;
    struct task* t;
    while(1) {
        pthread_mutex_lock(&(tp->mutex));
        // 无任务时，进入循环，等待唤醒
        while((tp->taskSize == 0) && !(tp->shutdown)) {
            pthread_cond_wait(&(tp->cond), &(tp->mutex));
        }
        // 立即关闭
        if(tp->shutdown == IMMEDIATE) {
            break;
        }
        // 优雅关闭
        else if(tp->shutdown == GRACEFUL && tp->taskSize == 0) {
            break;
        }
        
        // 去除任务
        t = tp->taskHead->next;
        if(t == NULL) {
            pthread_mutex_unlock(&(tp->mutex));
            continue;
        }
        tp->taskHead->next = t->next;
        tp->taskSize--;
        pthread_mutex_unlock(&(tp->mutex));
        
        // 执行任务
        (*(t->func))(t->args);

        free(t);
    }
    tp->started--;
    pthread_mutex_unlock(&(tp->mutex));
    pthread_exit(NULL);
}

void printErrType(char* errmsg) {
    printf("%s\n", errmsg);
}
