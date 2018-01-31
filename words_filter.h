#ifndef _WORDSFILTER_H
#define _WORDSFILTER_H

#include <stdint.h>

typedef struct wtoken { 
    char            code;             //字符的编码     
    struct wtoken** children;         //子节点
    uint32_t        children_size;    //子节点的数量
    uint8_t         end;              //是否一个word的结尾
}wtoken;


typedef struct words_filter {
    wtoken *tokarry[256];
}words_filter;


/*
*  初始化filter
*  @filter 要初始化的filter
*  @invaild_words 非法词列表，以NULL结尾
*/
int  words_filter_init(words_filter *filter,const char **invaild_words);

void words_filter_finalize(words_filter *filter);

/* 
*  判断输入串中是否存在非法词,如果不存在返回0否则返回-1
*/
int  words_check(words_filter *filter,const char *str,size_t str_size);

int  words_filtrate(words_filter *filter,const char *str,size_t str_size,char replace,char *out,size_t *out_size);


#endif