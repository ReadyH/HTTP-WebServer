//
// Created by huang on 2019/7/25.
//

#include "utils.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

void errorHandler(char* msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

int nonBlock(int fd) {

    int flag = 0;
    if((flag = fcntl(fd, F_GETFL, 0)) == -1) {
        errorHandler("fcntl() error");
    }

    flag = flag | O_RDWR | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1) {
        errorHandler("fcntl() error");
    }

    return 0;
}

size_t Writen(int fd, void *usrBuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char *)usrBuf;

    while(nleft > 0){
        if((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)
                nwritten = 0;
            else{
                return -1;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
