//
// Created by huang on 2019/7/25.
//

#ifndef PARSE_H
#define PARSE_H

#include "http.h"
#define CR '\r'
#define LF '\n'

#define HTTP_UNKNOWN 0x0001
#define HTTP_GET 0x0002
#define HTTP_HEAD 0x0004
#define HTTP_POST 0x0008
#define HTTP_OK  200
#define HTTP_NOT_MODIFIED 304
#define HTTP_NOT_FOUND 404

// http请求行解析
int httpParseRequestLine(httpRequest* request);
// http请求体解析
int httpParseRequestBody(httpRequest* request);

#endif //PARSE_H
