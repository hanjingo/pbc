#ifndef PROTOBUFC_PROTO_H
#define PROTOBUFC_PROTO_H

#include "pbc.h"
#include "map.h"
#include "array.h"
#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <stddef.h>

struct map_ip;
struct map_si;
struct map_sp;
struct _message;
struct _enum;

#define LABEL_OPTIONAL 0
#define LABEL_REQUIRED 1
#define LABEL_REPEATED 2
#define LABEL_PACKED 3
// message field
struct _field {
	int id;                     // 序号
	const char *name;           // 标签名
	int type;                   // 类型
	int label;                  // 关键字属性
	pbc_var default_v;          // 默认值
	union {
		const char * n;
		struct _message * m;
		struct _enum * e;
	} type_name;                // 类型名
};
// message
struct _message {
	const char * key;           // 
	struct map_ip * id;	        // id -> _field
	struct map_sp * name;	    // string -> _field
	struct pbc_rmessage * def;	// 默认值
	struct pbc_env * env;       // 
};
// enum
struct _enum {
	const char * key;           // 
	struct map_ip * id;         // 
	struct map_si * name;       // 
	pbc_var default_v;          // 默认值
};
// pbc env
struct pbc_env {
	struct map_sp * files;	    // string -> void *
	struct map_sp * enums;	    // 枚举map；key:类型名, value:_enum 
	struct map_sp * msgs;	    // 类型map；key:类型名, value:_message
	const char * lasterror;     // 
};

struct _message * _pbcP_init_message(struct pbc_env * p, const char *name);
void _pbcP_push_message(struct pbc_env * p, const char *name, struct _field *f , pbc_array queue);
struct _enum * _pbcP_push_enum(struct pbc_env * p, const char *name, struct map_kv *table, int sz );
int _pbcP_message_default(struct _message * m, const char * name, pbc_var defv);
struct _message * _pbcP_get_message(struct pbc_env * p, const char *name);
int _pbcP_type(struct _field * field, const char **type);

#endif
