
#ifndef MYSERVERCLION_EPOLL_H
#define MYSERVERCLION_EPOLL_H

#include <sys/epoll.h>
#include "threadpool.h"
#include "http.h"

#define MAX_EVENTS 1024

int createEpoll(int flags);
int addEpoll(int epollFd, int servSock, httpRequest* request, int events);
int epollWait(int epollFd, struct epoll_event* events, int maxEvents, int timeout);
int modifyEpoll(int epollFd, int servSock, httpRequest* request, int events);
int handleEvents(int epollFd, int servSock, struct epoll_event* events, int eventsNum, char* path, struct threadPool* tp);
#endif //MYSERVERCLION_EPOLL_H
