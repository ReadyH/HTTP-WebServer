//
// Created by huang on 2019/7/25.
//

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define strComp(m, c0, c1, c2, c3)                                       \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

void errorHandler(char* msg);
int nonBlock(int fd);
size_t Writen(int fd, void *usrBuf, size_t n);

#endif UTILS_H
