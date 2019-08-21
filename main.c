#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
#include "epoll.h"
#include "http.h"
#include "threadpool.h"
#include "heap.h"
#include "timer.h"

#define PORT 2048
#define THREAD_NUM 2
#define PATH "./"
#define MAX_LISTEN 1024
#define PQ_SIZE 10

extern struct epoll_event* events;
int handleEvents(int epollFd, int servSock, struct epoll_event* events, int eventsNum, char* path, struct threadPool* tp);

int main() {
    // 服务器基本配置
    int port = PORT;
    int threadNum = THREAD_NUM;
    char* path = PATH;
    int maxListen = MAX_LISTEN;
    int pqSize = PQ_SIZE;

    // 设置服务器套接字
    int servSock = 0;
    if((servSock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        errorHandler("socket() error");
    }

    // 设置套接字地址可重用
    int optval = 1;
    if(setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR,
                  (const void*)& optval, sizeof(optval)) == -1) {
        errorHandler("setsocketopt() error");
    }

    // 设置服务器IP和端口
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 绑定servAddr和servSock
    if(bind(servSock, (struct sockaddr*)& servAddr, sizeof(servAddr)) == -1) {
        errorHandler("bind() error");
    }

    // 监听servSock
    if(listen(servSock, maxListen) == -1) {
        errorHandler("listen() error");
    }
    // 设置文件非组塞
    nonBlock(servSock);
    
    // 初始化epollFd
    int epollFd = createEpoll(0);
    
    // 初始化HTTP请求
    httpRequest* request = (httpRequest*)malloc(sizeof(httpRequest));
    initRequest(request, servSock, epollFd, path);

    // 添加监听套接字到epoll
    addEpoll(epollFd, servSock, request, (EPOLLIN | EPOLLET));

    // 初始化线程池和定时器
    struct threadPool* tp = createThreadPool(threadNum);
    createTimer(pqSize);

    while(1) {
        getTime();
        // 得到事件触发个数
        int eventsNum = epollWait(epollFd, events, MAX_EVENTS, -1);

        handleExpireTimerNode();

        // 处理事件
        handleEvents(epollFd, servSock, events, eventsNum, path, tp);
    }
    return 0;
}

// 处理事件
int handleEvents(int epollFd, int servSock, struct epoll_event* events, int eventsNum, char* path, struct threadPool* tp) {
    for(int i = 0; i < eventsNum; ++i) {
        httpRequest* request = (httpRequest*)(events[i].data.ptr);
        int fd = request->fd;
        // 如果活跃事件是servSock，为新连接
        if(fd == servSock) {
            // 初始化客户端地址
            struct sockaddr_in clntAddr;
            memset(&clntAddr, 0, sizeof(clntAddr));
            socklen_t clntAddrLen = 0;
            int clntSock = 0;
            // 接受连接
            if((clntSock = accept(servSock, (struct sockaddr*)&clntAddr, &clntAddrLen)) == -1) {
                errorHandler("accept() error");
            }
            // 设置非阻塞
            if(nonBlock(clntSock) != 0) {
                errorHandler("non-block clntSock error");
            }
            // 初始化请求
            httpRequest* request = (httpRequest*)malloc(sizeof(httpRequest));
            initRequest(request, clntSock, epollFd, path);
            // 添加epoll事件
            addEpoll(epollFd, clntSock, request, (EPOLLIN | EPOLLET | EPOLLONESHOT));
            // 初始化计时器节点
            addTimerNode(request, TIMEOUT, httpClose);
        }
        // 数据传输
        else {
            // 排除错误状态
            if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
               || !(events[i].events & EPOLLIN)) {
                close(fd);
                continue;
            }
            // 开一个线程处理
            addThreadPool(tp, handleRequest, (void*)events[i].data.ptr);
        }
    }
    return 0;
}