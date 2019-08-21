//
// Created by huang on 2019/7/25.
//

#include "parse.h"
#include "http.h"
#include "utils.h"
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
int httpParseRequestLine(httpRequest *request){
    enum{
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_after_slash_in_uri,
        sw_http,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    }state;
    state = request->state;

    u_char ch, *p, *m;
    size_t pi;
    for(pi = request->pos; pi < request->last; pi++){
        p = (u_char *)&request->buff[pi % MAX_BUF];
        ch = *p;

        switch(state){
            case sw_start:
                request->requestStart = p;
                if(ch == CR || ch == LF)
                    break;
                if((ch < 'A' || ch > 'Z') && ch != '_') {
                    errorHandler("http request method invalid");
                }
                state = sw_method;
                break;

            case sw_method:
                if(ch == ' '){
                    request->methodEnd = p;
                    m = request->requestStart;
                    switch(p - m){
                        case 3:
                            if(strComp(m, 'G', 'E', 'T', ' ')){
                                request->method = HTTP_GET;
                                break;
                            }
                            break;
                        case 4:
                            if(strComp(m, 'P', 'O', 'S', 'T')){
                                request->method = HTTP_POST;
                                break;
                            }
                            if(strComp(m, 'H', 'E', 'A', 'D')){
                                request->method = HTTP_HEAD;
                                break;
                            }
                            break;
                        default:
                            request->method = HTTP_UNKNOWN;
                            break;
                    }
                    state = sw_spaces_before_uri;
                    break;
                }

                if((ch < 'A' || ch > 'Z') && ch != '_') {
                    errorHandler("http request method invalid");
                }
                break;

            case sw_spaces_before_uri:
                if(ch == '/'){
                    request->uriStart = p + 1;
                    state = sw_after_slash_in_uri;
                    break;
                }
                switch(ch){
                    case ' ':
                        break;
                    default:
                        errorHandler("http request invalid");
                }
                break;

            case sw_after_slash_in_uri:
                switch(ch){
                    case ' ':
                        request->uriEnd = p;
                        state = sw_http;
                        break;
                    default:
                        break;
                }
                break;

            case sw_http:
                switch(ch){
                    case ' ':
                        break;
                    case 'H':
                        state = sw_http_H;
                        break;
                    default:
                        errorHandler("http request invalid");
                }
                break;

            case sw_http_H:
                switch(ch){
                    case 'T':
                        state = sw_http_HT;
                        break;
                    default:
                        errorHandler("http request invalid");
                }
                break;

            case sw_http_HT:
                switch(ch){
                    case 'T':
                        state = sw_http_HTT;
                        break;
                    default:
                        errorHandler("http request invalid");
                }
                break;

            case sw_http_HTT:
                switch(ch){
                    case 'P':
                        state = sw_http_HTTP;
                        break;
                    default:
                        errorHandler("http request invalid");
                }
                break;

            case sw_http_HTTP:
                switch(ch){
                    case '/':
                        state = sw_first_major_digit;
                        break;
                    default:
                        errorHandler("http request invalid");
                }
                break;

            case sw_first_major_digit:
                if(ch < '1' || ch > '9')
                    errorHandler("http request invalid");
                request->httpMajor = ch - '0';
                state = sw_major_digit;
                break;

            case sw_major_digit:
                if(ch == '.'){
                    state = sw_first_minor_digit;
                    break;
                }
                if(ch < '0' || ch > '9')
                    errorHandler("http request invalid");
                request->httpMajor = request->httpMajor * 10 + ch - '0';
                break;

            case sw_first_minor_digit:
                if(ch < '0' || ch > '9')
                    errorHandler("http request invalid");
                request->httpMinor = ch - '0';
                state = sw_minor_digit;
                break;

            case sw_minor_digit:
                if(ch == CR){
                    state = sw_almost_done;
                    break;
                }
                if(ch == LF)
                    goto done;
                if(ch == ' '){
                    state = sw_spaces_after_digit;
                    break;
                }
                if(ch < '0' || ch > '9')
                    errorHandler("http request invalid");
                request->httpMinor = request->httpMinor * 10 + ch - '0';
                break;

            case sw_spaces_after_digit:
                switch(ch){
                    case ' ':
                        break;
                    case CR:
                        state = sw_almost_done;
                        break;
                    case LF:
                        goto done;
                    default:
                        errorHandler("http request invalid");
                }
                break;

            case sw_almost_done:
                request->requestEnd = p - 1;
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        errorHandler("http request invalid");
                }
        }
    }
    request->pos = pi;
    request->state = state;
    return EAGAIN;

    done:
    request->pos = pi + 1;
    if (request->requestEnd == NULL)
        request->requestEnd = p;
    request->state = sw_start;
    return 0;
}

int httpParseRequestBody(httpRequest *request){
    // 状态列表
    enum{
        sw_start = 0,
        sw_key,
        sw_spaces_before_colon,
        sw_spaces_after_colon,
        sw_value,
        sw_cr,
        sw_crlf,
        sw_crlfcr
    }state;
    state = request->state;

    size_t pi;
    unsigned char ch, *p;
    httpHeader *hd;
    for (pi = request->pos; pi < request->last; pi++) {
        p = (unsigned char*)&request->buff[pi % MAX_BUF];
        ch = *p;

        switch(state){
            case sw_start:
                if(ch == CR || ch == LF)
                    break;
                request->curHeaderKeyStart = p;
                state = sw_key;
                break;

            case sw_key:
                if(ch == ' '){
                    request->curHeaderKeyEnd = p;
                    state = sw_spaces_before_colon;
                    break;
                }
                if(ch == ':'){
                    request->curHeaderKeyEnd = p;
                    state = sw_spaces_after_colon;
                    break;
                }
                break;

            case sw_spaces_before_colon:
                if(ch == ' ')
                    break;
                else if(ch == ':'){
                    state = sw_spaces_after_colon;
                    break;
                }
                else
                    errorHandler("http request header invalid");

            case sw_spaces_after_colon:
                if (ch == ' ')
                    break;
                state = sw_value;
                request->curHeaderValueStart = p;
                break;

            case sw_value:
                if(ch == CR){
                    request->curHeaderValueEnd = p;
                    state = sw_cr;
                }
                if(ch == LF){
                    request->curHeaderValueEnd = p;
                    state = sw_crlf;
                }
                break;

            case sw_cr:
                if(ch == LF){
                    state = sw_crlf;
                    hd = (httpHeader*) malloc(sizeof(httpHeader));
                    hd->keyStart = request->curHeaderKeyStart;
                    hd->keyEnd = request->curHeaderKeyEnd;
                    hd->valueStart = request->curHeaderValueStart;
                    hd->valueEnd = request->curHeaderValueEnd;
                    list_add(&(hd->list), &(request->list));
                    break;
                }
                else
                    errorHandler("http request header invalid");

            case sw_crlf:
                if(ch == CR)
                    state = sw_crlfcr;
                else{
                    request->curHeaderKeyStart = p;
                    state = sw_key;
                }
                break;

            case sw_crlfcr:
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        errorHandler("http request header invalid");
                }
        }
    }
    request->pos = pi;
    request->state = state;
    return EAGAIN;

    done:
    request->pos = pi + 1;
    request->state = sw_start;
    return 0;
}