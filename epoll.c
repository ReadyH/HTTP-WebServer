#include "epoll.h"

#include "epoll.h"
#include "utils.h"
#include "timer.h"
#include "threadpool.h"
#include "socket.h"
#include "http.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

struct epoll_event* events;

// 初始化epoll
int createEpoll(int flags) {
    int epollFd = epoll_create1(flags);
    if(epollFd == -1) {
        errorHandler("epoll_create1() error");
    }
    events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    return epollFd;
}

// 添加事件fd
int addEpoll(int epollFd, int fd, httpRequest* request, int events) {
    struct epoll_event event;
    event.data.ptr = (void*)request;
    event.events = events;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
        errorHandler("epoll_ctl() error!");
    }
    return 0;
}

// 等待事件
int epollWait(int epollFd, struct epoll_event* events, int maxEvents, int timeout) {
    int eventsNum = epoll_wait(epollFd, events, maxEvents, timeout);
    return eventsNum;
}

// 修改事件
int modifyEpoll(int epollFd, int fd, httpRequest* request, int events) {
    struct epoll_event event;
    event.data.ptr = (void*)request;
    event.events = events;
    if(epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) == -1) {
        errorHandler("modify epoll failed!");
    }

    return 0;
}