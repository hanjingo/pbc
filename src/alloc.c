#include <stdlib.h>
#include <stdio.h>
// 用于统计内存的申请和释放次数匹配
static int _g = 0;

void * _pbcM_malloc(size_t sz) {
	++ _g;
	return malloc(sz);
}

void _pbcM_free(void *p) {
	if (p) {
		-- _g;
		free(p);
	}
}

void* _pbcM_realloc(void *p, size_t sz) {
	return realloc(p,sz);
}

void _pbcM_memory() {
	printf("%d\n",_g);	
}
// 堆页（用于记录每个内存堆的首地址）
struct heap_page {
	struct heap_page * next;
};
// 堆页链表
struct heap {
	struct heap_page *current; // 堆页地址
	int size;                  // 总内存大小
	int used;                  // 已使用内存大小
};
// 新建一个堆页链表；pagesize:指定堆页尺寸
struct heap * 
_pbcH_new(int pagesize) {
	int cap = 1024;
	while(cap < pagesize) { // 保证新建的堆页链表总内存是堆页尺寸的整数倍
		cap *= 2;
	}
	struct heap * h = (struct heap *)_pbcM_malloc(sizeof(struct heap));
	h->current = (struct heap_page *)_pbcM_malloc(sizeof(struct heap_page) + cap); // 初始是指向整个堆页链表的头
	h->size = cap;
	h->used = 0;
	h->current->next = NULL;
	return h;
}
// 遍历heap_page并全部删除
void 
_pbcH_delete(struct heap *h) {
	struct heap_page * p = h->current;
	struct heap_page * next = p->next;
	for(;;) {
		_pbcM_free(p);
		if (next == NULL)
			break;
		p = next;
		next = p->next;
	}
	_pbcM_free(h);
}
// 分配heap的空间，返回分配的内存块； h:堆，size:需要分配的空间
void* 
_pbcH_alloc(struct heap *h, int size) {
	size = (size + 3) & ~3; // size = size / 4
	if (h->size - h->used < size) { // 剩余空间不够分配
		struct heap_page * p;
		if (size < h->size) { // 总空间 > 需要分配的空间
			p = (struct heap_page *)_pbcM_malloc(sizeof(struct heap_page) + h->size); // 直接使用
		} else {
			p = (struct heap_page *)_pbcM_malloc(sizeof(struct heap_page) + size); // 扩容
		}
		p->next = h->current;
		h->current = p;
		h->used = size;
		return (p+1);
	} else { // 剩余空间足够
		char * buffer = (char *)(h->current + 1);
		buffer += h->used; // 跳过已使用的空间
		h->used += size;   // 分出去的这部分做记录
		return buffer;
	}
}
