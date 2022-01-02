#ifndef PROTOBUF_C_PATTERN_H
#define PROTOBUF_C_PATTERN_H

#include "pbc.h"
#include "context.h"
#include "array.h"
// C语言结构体匹配域
struct _pattern_field {
	int id;               // id
	int offset;           // 偏移量
	int ptype;            // 匹配类型
	int ctype;            // C语言类型
	int label;            // 标签值
	pbc_var defv;         // 默认值
};
// C语言结构体匹配域的一层封装
struct pbc_pattern {
	struct pbc_env * env;       // 所属的pbc env
	int count;                  // 数量
	struct _pattern_field f[1]; // 指针
};

struct pbc_pattern * _pbcP_new(struct pbc_env * env, int n);
int _pbcP_unpack_packed(uint8_t *buffer, int size, int ptype, pbc_array array);

#endif
