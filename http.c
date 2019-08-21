
#include "http.h"
#include "utils.h"
#include "timer.h"
#include "epoll.h"
#include "parse.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char *ROOT = NULL;


// 初始化请求结构
int initRequest(httpRequest* request, int fd, int epoll_fd, char* path) {
    request->fd = fd;
    request->epollFd = epoll_fd;
    request->pos = 0;
    request->last = 0;
    request->state = 0;
    request->root = path;
    INIT_LIST_HEAD(&(request->list));
    return 0;
}

// 初始化输出结构
int initHttpOut(httpOut* out, int fd){
    out->fd = fd;
    out->keepAlive = 1;
    out->status = 200;
    out->modified = 1;

    return 0;
}

// 错误处理函数集合
headerHandler hHandlers[] = {
        {"Host", httpIgnore},
        {"Connection", httpConnection},
        {"If-Modified-Since", httpIfModifiedSince},
        {"", httpIgnore}
};

mimeType mime[] = {
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt", "text/plain"},
        {".rtf", "application/rtf"},
        {".pdf", "application/pdf"},
        {".word", "application/msword"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".au", "audio/basic"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"},
        {".css", "text/css"},
        {NULL ,"text/plain"}
};

// 处理请求头
void handleHttpHeader(httpRequest* request, httpOut* out){
    list_head* pos;
    httpHeader* hd;
    headerHandler* hHandler;
    int len;
    list_for_each(pos, &(request->list)){
        hd = list_entry(pos, httpHeader, list);
        for(hHandler = hHandlers; strlen(hHandler->name) > 0; hHandler++){
            if(strncmp(hd->keyStart, hHandler->name, hd->keyEnd - hd->keyStart) == 0){
                len = hd->valueEnd - hd->valueStart;
                (*(hHandler->handler))(request, out, hd->valueStart, len);
                break;
            }
        }
        list_del(pos);
        free(hd);
    }
}

// 关闭连接
int httpClose(httpRequest* request){
    close(request->fd);
    //free(request); segment fault
    return 0;
}

// 忽略链接
int httpIgnore(httpRequest* request, httpOut* out, char* data, int len) {
    (void) request;
    (void) out;
    (void) data;
    (void) len;
    return 0;
}

int httpConnection(httpRequest* request, httpOut* out, char* data, int len) {
    (void) request;

    if (strncasecmp("keep-alive", data, len) == 0) {
        out->keepAlive = 1;
    }
    return 0;
}

// 返回修改情况
int httpIfModifiedSince(httpRequest* request, httpOut* out, char* data, int len) {
    (void) request;
    (void) len;
    struct tm tm;
    // 转换data格式为GMT格式
    if(strptime(data, "%a, %d %b %Y %H:%M:%S GMT", &tm) == (char*)NULL){
        return 0;
    }
    // 将时间转换为自1970年1月1日以来持续时间的秒数
    time_t clientTime = mktime(&tm);
    // 计算两个时刻之间的时间差
    double timeDiff = difftime(out->mtime, clientTime);
    // 微秒时间内未修改status显示未修改，modify字段置为1
    if(fabs(timeDiff) < 1e-6){
        out->modified = 0;
        out->status = HTTP_NOT_MODIFIED;
    }
    return 0;
}

// 返回信息
const char* getMsg(int statusCode){
    if(statusCode == HTTP_OK){
        return "OK";
    }
    if(statusCode == HTTP_NOT_MODIFIED){
        return "Not Modified";
    }
    if(statusCode == HTTP_NOT_FOUND){
        return "Not Found";
    }
    return "Unknown";
}

// 返回文件类型
const char* getFileType(const char* type){
    // 将type和索引表中后缀比较，返回类型用于填充Content-Type字段
    for(int i = 0; mime[i].type != NULL; ++i){
        if(strcmp(type, mime[i].type) == 0)
            return mime[i].value;
    }
    // 未识别返回"text/plain"
    return "text/plain";
}

// 静态响应
void handleStatic(int fd, char* filename, size_t filesize, httpOut* out){
    // 响应头缓冲（512字节）和数据缓冲（8192字节）
    char header[512];
    char buff[8192];
    struct tm tm;

    // 返回响应报文头，包含HTTP版本号状态码及状态码对应的短描述
    sprintf(header, "HTTP/1.1 %d %s\r\n", out->status, getMsg(out->status));

    // 返回响应头
    // Connection，Keep-Alive，Content-type，Content-length，Last-Modified
    if(out->keepAlive){
        // 返回默认的持续连接模式及超时时间（默认500ms）
        sprintf(header, "%sConnection: keep-alive\r\n", header);
        sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, TIMEOUT);
    }

    if(out->modified){
        // 得到文件类型并填充Content-type字段
        const char* filetype = getFileType(strrchr(filename, '.'));
        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        // 通过Content-length返回文件大小
        sprintf(header, "%sContent-length: %zu\r\n", header, filesize);
        // 得到最后修改时间并填充Last-Modified字段
        localtime_r(&(out->mtime), &tm);
        strftime(buff, 512,  "%a, %d %b %Y %H:%M:%S GMT", &tm);
        sprintf(header, "%sLast-Modified: %s\r\n", header, buff);
    }

    sprintf(header, "%sServer\r\n", header);

    // 空行（must）
    sprintf(header, "%s\r\n", header);

    // 发送报文头部并校验完整性
    size_t sendLen = (size_t)Writen(fd, header, strlen(header));

    if(sendLen != strlen(header)){

        perror("Send header failed");
        return;
    }

    // 打开并发送文件
    int srcFd = open(filename, O_RDONLY, 0);

    char *srcAddr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcFd, 0);
    close(srcFd);

    // 发送文件并校验完整性
    sendLen = Writen(fd, srcAddr, filesize);
    if(sendLen != filesize){
        perror("Send file failed");
        return;
    }

    munmap(srcAddr, filesize);
}

// 处理出错情况
int errorProcess(struct stat* sbufptr, char *filename, int fd) {
    if(stat(filename, sbufptr) < 0){
        handleError(fd, filename, "404", "Not Found", "can't find the file");
        return 1;
    }

    // 处理权限错误
    if(!(S_ISREG((*sbufptr).st_mode)) || !(S_IRUSR & (*sbufptr).st_mode)){
        handleError(fd, filename, "403", "Forbidden", "can't read the file");
        return 1;
    }

    return 0;
}

// 返回错误信息
void handleError(int fd, char *cause, char *errNum, char *shortMsg, char *longMsg) {
    // 响应头缓冲（512字节）和数据缓冲（8192字节）
    char header[512];
    char body[8192];

    // 用log_msg和cause字符串填充错误响应体
    sprintf(body, "<html><title>Server Error<title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\n", body);
    sprintf(body, "%s%s : %s\n", body, errNum, shortMsg);
    sprintf(body, "%s<p>%s : %s\n</p>", body, longMsg, cause);
    sprintf(body, "%s<hr><em>Server</em>\n</body></html>", body);

    // 返回错误码，组织错误响应头
    sprintf(header, "HTTP/1.1 %s %s\r\n", errNum, shortMsg);
    sprintf(header, "%sServer:\r\n", header);
    sprintf(header, "%sContent-type: text/html\r\n", header);
    sprintf(header, "%sConnection: close\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));

    // Add 404 Page

    // 发送错误信息
    Writen(fd, header, strlen(header));
    Writen(fd, body, strlen(body));
    return;
}

// 解析URI
void parseURI(char *uriStart, int uriLength, char *filename) {
    uriStart[uriLength] = '\0';

    char *delimPos = strchr(uriStart, '?');
    int filenameLength = (delimPos != NULL) ? ((int)(delimPos - uriStart)) : uriLength;
    strcpy(filename, ROOT);

    strncat(filename, uriStart, filenameLength);

    char *lastComp = strrchr(filename, '/');

    char *lastDot = strrchr(lastComp, '.');

    if((lastDot == NULL) && (filename[strlen(filename) - 1] != '/')){
        strcat(filename, "/");
    }

    if(filename[strlen(filename) - 1] == '/'){
        strcat(filename, "index.html");
    }
}

// 处理请求
void handleRequest(void* ptr) {
    // 请求基本信息
    httpRequest* request = (httpRequest*)(ptr);
    int fd = request->fd;
    ROOT = request->root;
    char filename[512];
    struct stat sbuf;
    int res = 0, readNum = 0, errType = 0;;
    char* writable = NULL;
    size_t leftSize = 0;
    
    // 删除节点
    deleteTimerNode(request);

    while(1) {
        // 指向缓冲区可写入的第一个位置
        writable = &request->buff[request->last % MAX_BUF];
        size_t left1 = MAX_BUF - (request->last - request->pos) - 1;
        size_t left2 = MAX_BUF - (request->last % MAX_BUF);
        // leftSize表示可写的剩余大小
        if(left1 < left2) leftSize = left1;
        else leftSize = left2;
        
        // 读取连接描述符的数据
        readNum = read(fd, writable, leftSize);
        // 若无数据可读，则断开连接
        if(readNum == 0) {
            errType = 1;
            break;
        }
        // 若读取错误，则断开连接
        if(readNum < 0 && errno != EAGAIN) {
            errType = 1;
            break;
        }
        // 若不是读写错误，而是非阻塞下数据未到达，则不断开连接，继续等待请求
        if(readNum < 0 && errno == EAGAIN) {
            break;
        }
        // 更新可写位置
        request->last += readNum;

        // 解析请求报文行
        res = httpParseRequestLine(request);
        if(res == EAGAIN) continue;
        else if(res != 0) {
            errType = 1;
            break;
        }

        // 解析请求报文体
        res = httpParseRequestBody(request);
        if(res == EAGAIN) continue;
        else if(res != 0) {
            errType = 1;
            break;
        }

        // 初始化返回结构
        httpOut* out = (httpOut*)malloc(sizeof(httpOut));
        initHttpOut(out, fd);

        // 解析URI，获取文件名
        parseURI(request->uriStart, (request->uriEnd - request->uriStart), filename);

        // 处理错误，如果发生
        if(errorProcess(&sbuf, filename, fd)) continue;

        // 处理返回结构
        handleHttpHeader(request, out);

        // 获取文件类型
        out->mtime = sbuf.st_mtime;

        // 处理文件请求
        handleStatic(fd, filename, sbuf.st_size, out);

        // 若不是长连接请求，则断开连接
        if(!out->keepAlive) {
            errType = 1;
            break;
        }
        else {
            free(out);
        }



    }
    // 长连接情况下，重置状态（epoll、定时器）
    if(errType == 0) {
        modifyEpoll(request->epollFd, request->fd, request, (EPOLLIN | EPOLLET | EPOLLONESHOT));
        addTimerNode(request, TIMEOUT, httpClose);
    }
    // 其他情况，断开连接
    if(errType == 1) {
        httpClose(request);
        //close(fd);
    }
}
