
#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "list.h"

#define MAX_BUF 8124

typedef struct httpRequest{
    char* root;
    int fd;
    int epollFd;
    char buff[MAX_BUF];
    size_t pos;
    size_t last;
    int state;

    void* requestStart;
    void* methodEnd;
    int method;
    void* uriStart;
    void* uriEnd;
    void* pathStart;
    void* pathEnd;
    void* queryStart;
    void* queryEnd;
    int httpMajor;
    int httpMinor;
    void* requestEnd;

    struct list_head list;

    void* curHeaderKeyStart;
    void* curHeaderKeyEnd;
    void* curHeaderValueStart;
    void* curHeaderValueEnd;
    void* timer;
}httpRequest;

typedef struct httpOut {
    int fd;
    int keepAlive;
    time_t mtime;
    int modified;
    int status;
}httpOut;

typedef struct httpHeader{
    void* keyStart;
    void* keyEnd;
    void* valueStart;
    void* valueEnd;
    struct list_head list;
}httpHeader;

typedef int (*httpHeaderHandler)(httpRequest* request, httpOut* out, char* data, int len);

typedef struct headerHandler{
    char* name;
    httpHeaderHandler handler;
}headerHandler;

typedef struct mimeType{
    const char *type;
    const char *value;
}mimeType;

int initRequest(httpRequest* request, int fd, int epoll_fd, char* path);
int initOut(httpOut* out, int fd);
int httpClose(httpRequest* request);
void handleRequest(void* ptr);
void handleStatic(int fd, char* filename, size_t filesize, httpOut* out);


int httpIgnore(httpRequest* request, httpOut* out, char* data, int len);
int httpConnection(httpRequest* request, httpOut* out, char* data, int len);
int httpIfModifiedSince(httpRequest* request, httpOut* out, char* data, int len);
void parseURI(char *uriStart, int uriLength, char *filename);
const char* getMsg(int statusCode);
const char* getFileType(const char* type);
int errorProcess(struct stat* sbufptr, char *filename, int fd);
void handleError(int fd, char *cause, char *errNum, char *shortMsg, char *longMsg);

#endif //HTTP_H
