#ifndef PROTOBUF_C_ARRAY_H
#define PROTOBUF_C_ARRAY_H

#include "varint.h"
#include "pbc.h"
#include "alloc.h"

typedef union _pbc_var {
	struct longlong integer; // 长整型
	double real;             // 浮点型
	struct {
		const char * str;
		int len;
	} s;                     // 自制string类型
	struct {
		int id;
		const char * name;
	} e;                     // ？
	struct pbc_slice m;      // 切片
	void * p[2];             // 任意类型
} pbc_var[1];

void _pbcA_open(pbc_array);
void _pbcA_open_heap(pbc_array, struct heap *h);
void _pbcA_close(pbc_array);

void _pbcA_push(pbc_array, pbc_var var);
void _pbcA_index(pbc_array , int idx, pbc_var var);
void * _pbcA_index_p(pbc_array _array, int idx);

#endif
