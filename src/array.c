#include "pbc.h"
#include "array.h"
#include "alloc.h"

#include <stdlib.h>
#include <string.h>
// 数组
struct array {
	int number;         // 数组长度
	struct heap *heap;  // 内存区
	union _pbc_var * a; // 任意类型
};
// 内部域 = (数组尺寸:64 - 数组大小) / 任意类型大小
#define INNER_FIELD ((PBC_ARRAY_CAP - sizeof(struct array)) / sizeof(pbc_var))
// 初始化数组
void 
_pbcA_open(pbc_array _array) {
	struct array * a = (struct array *)_array;
	a->number = 0;
	a->heap = NULL;
	a->a = (union _pbc_var *)(a+1);
}
// 绑定数组和堆
void 
_pbcA_open_heap(pbc_array _array, struct heap *h) {
	struct array * a = (struct array *)_array;
	a->number = 0;
	a->heap = h;
	a->a = (union _pbc_var *)(a+1);
}
// 关闭数组
void 
_pbcA_close(pbc_array _array) {
	struct array * a = (struct array *)_array;
	if (a->heap == NULL && a->a != NULL && (union _pbc_var *)(a+1) != a->a) {
		_pbcM_free(a->a);
		a->a = NULL;
	}
}
// 向数组尾部导入值(如果内存不够就扩容)
void 
_pbcA_push(pbc_array _array, pbc_var var) {
	struct array * a = (struct array *)_array;
	if (a->number == 0) {
		a->a = (union _pbc_var *)(a+1);
	} else if (a->number >= INNER_FIELD) {
		if (a->number == INNER_FIELD) { // 已用数量==剩余数量
			int cap = 1;
			while (cap <= a->number + 1) 
				cap *= 2; // 扩容2倍
			struct heap * h = a->heap;
			union _pbc_var * outer = (union _pbc_var *)HMALLOC(cap * sizeof(union _pbc_var));
			memcpy(outer , a->a , INNER_FIELD * sizeof(pbc_var));
			a->a = outer;
		} else {
			int size=a->number;
			if (((size + 1) ^ size) > size) { // 必须保证size+1的时候出现最高位进位时才成立
				struct heap * h = a->heap;
				if (h) {
					void * old = a->a;
					a->a = (union _pbc_var *)_pbcH_alloc(h, sizeof(union _pbc_var) * (size+1) * 2);
					memcpy(a->a, old, sizeof(union _pbc_var) * size);
				} else {
					a->a = (union _pbc_var *)_pbcM_realloc(a->a,sizeof(union _pbc_var) * (size+1) * 2);
				}
			}
		}
	}
	a->a[a->number] = *var;
	++ a->number;
}
// 对数组进行索引，通过参数返回（没有判断数组长度，需要使用者自行保证索引在数组范围内）
void 
_pbcA_index(pbc_array _array, int idx, pbc_var var)
{
	struct array * a = (struct array *)_array;
	var[0] = a->a[idx];
}
// 对数组进行索引，并通过返回值返回（没有判断数组长度，需要使用者自行保证索引在数组范围内）
void *
_pbcA_index_p(pbc_array _array, int idx)
{
	struct array * a = (struct array *)_array;
	return &(a->a[idx]);
}
// 返回数组长度
int 
pbc_array_size(pbc_array _array) {
	struct array * a = (struct array *)_array;
	return a->number;
}
// 根据索引取整型值，并返回低位值；array:数组，index:索引，hi:高位
uint32_t 
pbc_array_integer(pbc_array array, int index, uint32_t *hi) {
	pbc_var var;
	_pbcA_index(array , index , var);
	if (hi) {
		*hi = var->integer.hi; // 高位
	}
	return var->integer.low; // 低位
}
// 根据索引取浮点值；array:数组，index:索引
double 
pbc_array_real(pbc_array array, int index) {
	pbc_var var;
	_pbcA_index(array , index , var);
	return var->real;
}
// 取索引值及后面的切片；_array:数组，index:索引
struct pbc_slice *
pbc_array_slice(pbc_array _array, int index) {
	struct array * a = (struct array *)_array;
	if (index <0 || index > a->number) {
		return NULL;
	}
	return (struct pbc_slice *) &(a->a[index]);
}
// 向数组后面添加整型
void 
pbc_array_push_integer(pbc_array array, uint32_t low, uint32_t hi) {
	pbc_var var;
	var->integer.low = low;
	var->integer.hi = hi;
	_pbcA_push(array,var);
}
// 向数组后面添加切片
void 
pbc_array_push_slice(pbc_array array, struct pbc_slice *s) {
	pbc_var var;
	var->m = *s;
	_pbcA_push(array,var);
}
// 向数组后面添加浮点型
void 
pbc_array_push_real(pbc_array array, double v) {
	pbc_var var;
	var->real = v;
	_pbcA_push(array,var);
}
