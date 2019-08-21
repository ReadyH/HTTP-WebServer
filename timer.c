#include <sys/time.h>
#include "timer.h"
#include "heap.h"
#include <stdlib.h>

size_t currentMsec;
struct Heap theTimer;

// 初始化定时器
int createTimer(int pqSize) {
    initialize(&theTimer, pqSize);
    updateCurrentTime();
    return 0;
}

// 更新当前时间
void updateCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    currentMsec = ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

// 返回最早事件节点与当前时间差
int getTime() {
    int time = 0;
    while(!isEmpty(&theTimer)) {
        updateCurrentTime();
        T timerNode = findMin(&theTimer);
        if(timerNode->deleted) {
            deleteMin(&theTimer);
            free(timerNode);
            continue;
        }

        time = (int)(timerNode->key - currentMsec);
        time = (time > 0) ? time : 0;
        break;
    }
    return time;
}

// 添加定时器节点
void addTimerNode(httpRequest* request, size_t timeout, timeoutHandler handler) {
    updateCurrentTime();

    T timerNode = (struct timer*)malloc(sizeof(struct timer));
    request->timer = timerNode;
    timerNode->key = currentMsec + timeout;
    timerNode->deleted = 0;
    timerNode->handler = handler;
    timerNode->request = request;
    insert(timerNode, &theTimer);
}

// 删除节点（惰性）
void deleteTimerNode(httpRequest* request) {

    updateCurrentTime();

    struct timer* timerNode = (struct timer*)(request->timer);
    timerNode->deleted = 1;

}

// 处理超时结点
void handleExpireTimerNode() {
    while(!isEmpty(&theTimer)) {
        updateCurrentTime();
        T timerNode = findMin(&theTimer);
        if(timerNode->deleted) {
            deleteMin(&theTimer);
            free(timerNode);
            continue;
        }
        if(timerNode->key > currentMsec) {
            return;
        }

        if(timerNode->handler) {
            timerNode->handler(timerNode->request);
        }
        deleteMin(&theTimer);
        free(timerNode);
    }
}
