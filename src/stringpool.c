#include "alloc.h"

#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256

struct _stringpool {
	char * buffer;
	size_t len;
	struct _stringpool *next;
};

struct _stringpool * 
_pbcS_new(void) {
	struct _stringpool * ret = (struct _stringpool *)malloc(sizeof(struct _stringpool) + PAGE_SIZE);
	ret->buffer = (char *)(ret + 1);
	ret->len = 0;
	ret->next = NULL;
	return ret;
}
// 释放string池
void 
_pbcS_delete(struct _stringpool *pool) {
	while(pool) {
		struct _stringpool *next = pool->next;
		free(pool);
		pool = next;
	}
}
// 将str放入stringpool，并返回其内容; str:字符串, sz:字符串长度
const char *
_pbcS_build(struct _stringpool *pool, const char * str , int sz) {
	size_t s = sz + 1;
	if (s < PAGE_SIZE - pool->len) { // 当前页空闲空间足够，直接放入
		char * ret = pool->buffer + pool->len;
		memcpy(pool->buffer + pool->len, str, s);
		pool->len += s;
		return ret;
	}
	if (s > PAGE_SIZE) { // 字符串长度超过了页空间，新建stringpool
		struct _stringpool * next = (struct _stringpool *)malloc(sizeof(struct _stringpool) + s);
		next->buffer = (char *)(next + 1);
		memcpy(next->buffer, str, s);
		next->len = s;
		next->next = pool->next;
		pool->next = next;
		return next->buffer;
	}
	struct _stringpool *next = (struct _stringpool *)malloc(sizeof(struct _stringpool) + PAGE_SIZE); // 页空闲空间不够，新建stringpool
	next->buffer = pool->buffer;
	next->next = pool->next;
	next->len = pool->len;

	pool->next = next;
	pool->buffer = (char *)(next + 1);
	memcpy(pool->buffer, str, s);
	pool->len = s;
	return pool->buffer;
}
