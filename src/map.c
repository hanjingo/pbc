#include "map.h"
#include "alloc.h"

#include <stdlib.h>
#include <string.h>
// id-pointer槽
struct _pbcM_ip_slot {
	int id;              // id
	void * pointer;      // 指针
	int next;            // 指向下一个
};
// 整数映射到指针的静态哈希表（不支持扩容）
struct map_ip {
	size_t array_size;             // 数组长度
	void ** array;                 // 数组
	size_t hash_size;              // hash长度
	struct _pbcM_ip_slot * slot;   // id-pointer槽链表
};
// string-id槽
struct _pbcM_si_slot {
	const char *key;                // string key
	size_t hash;                    // hash长度
	int id;                         // id
	int next;                       // 指向下一个
};
// 字符串映射到id的静态哈希表
struct map_si {
	size_t size;                    // 数量
	struct _pbcM_si_slot slot[1];   // 槽指针（[1]表示指针）
};
// 计算hash
static size_t
calc_hash(const char *name)
{
	size_t len = strlen(name);
	size_t h = len;
	size_t step = (len>>5)+1;
	size_t i;
	for (i=len; i>=step; i-=step)
	    h = h ^ ((h<<5)+(h>>2)+(size_t)name[i-1]);
	return h;
}
// 新建一个字符串映射到id的静态哈希表；table:键值对哈希表（用来提供初始化值）, size:键值对哈希表容量
struct map_si *
_pbcM_si_new(struct map_kv * table, int size)
{
	size_t sz = sizeof(struct map_si) + (size-1) * sizeof(struct _pbcM_si_slot);
	struct map_si * ret = (struct map_si *)malloc(sz);
	memset(ret,0,sz);

	ret->size = (size_t)size;

	int empty = 0;
	int i;

	for (i=0;i<size;i++) { // 遍历并赋值
		size_t hash_full = calc_hash((const char *)table[i].pointer);
		size_t hash = hash_full % size;
		struct _pbcM_si_slot * slot = &ret->slot[hash];
		if (slot->key == NULL) {
			slot->key = (const char *)table[i].pointer;
			slot->id = table[i].id;
			slot->hash = hash_full;
		} else {
			while(ret->slot[empty].key != NULL) {
				++empty;
			}
			struct _pbcM_si_slot * empty_slot = &ret->slot[empty];
			empty_slot->next = slot->next;
			slot->next = empty + 1;
			empty_slot->id = table[i].id;
			empty_slot->key = (const char *)table[i].pointer;
			empty_slot->hash = hash_full;
		}
	}

	return ret;
}
// 删除一个字符串映射到id的静态哈希表
void 
_pbcM_si_delete(struct map_si *map)
{
	free(map);
}
// 根据key查询标签序号，0表示未找到；map:待查询的序号表, key:键, result:返回的结果
int
_pbcM_si_query(struct map_si *map, const char *key, int *result) 
{
	size_t hash_full = calc_hash(key);
	size_t hash = hash_full % map->size;

	struct _pbcM_si_slot * slot = &map->slot[hash];
	if (slot->key == NULL) {
		return 1;
	}
	for (;;) {
		if (slot->hash == hash_full && strcmp(slot->key, key) == 0) {
			*result = slot->id;
			return 0;
		}
		if (slot->next == 0) {
			return 1;
		}
		slot = &map->slot[slot->next-1];
	}
}
// 新建一个id-pointer map并hash化
static struct map_ip *
_pbcM_ip_new_hash(struct map_kv * table, int size)
{
	struct map_ip * ret = (struct map_ip *)malloc(sizeof(struct map_ip));
	ret->array = NULL;
	ret->array_size = 0;
	ret->hash_size = (size_t)size;
	ret->slot = (struct _pbcM_ip_slot *)malloc(sizeof(struct _pbcM_ip_slot) * size);
	memset(ret->slot,0,sizeof(struct _pbcM_ip_slot) * size);
	int empty = 0;
	int i;
	for (i=0;i<size;i++) {
		int hash = ((unsigned)table[i].id) % size;
		struct _pbcM_ip_slot * slot = &ret->slot[hash];
		if (slot->pointer == NULL) {
			slot->pointer = table[i].pointer;
			slot->id = table[i].id;
		} else {
			while(ret->slot[empty].pointer != NULL) {
				++empty;
			}
			struct _pbcM_ip_slot * empty_slot = &ret->slot[empty];
			empty_slot->next = slot->next;
			slot->next = empty + 1;
			empty_slot->id = table[i].id;
			empty_slot->pointer = table[i].pointer;
		}
	}
	return ret;
}
// 新建一个id-pointer map
struct map_ip *
_pbcM_ip_new(struct map_kv * table, int size)
{
	int i;
	int max = table[0].id;
	if (max > size * 2 || max < 0)
		return _pbcM_ip_new_hash(table,size);
	for (i=1;i<size;i++) {
		if (table[i].id < 0) {
			return _pbcM_ip_new_hash(table,size);
		}
		if (table[i].id > max) {
			max = table[i].id;
			if (max > size * 2)
				return _pbcM_ip_new_hash(table,size);
		}
	}
	struct map_ip * ret = (struct map_ip *)malloc(sizeof(struct map_ip));
	ret->hash_size = size;
	ret->slot = NULL;
	ret->array_size = max + 1;
	ret->array = (void **)malloc((max+1) * sizeof(void *));
	memset(ret->array,0,(max+1) * sizeof(void *));
	for (i=0;i<size;i++) {
		ret->array[table[i].id] = table[i].pointer;
	}
	return ret;
}
// 删除一个id-pointer map
void
_pbcM_ip_delete(struct map_ip * map)
{
	if (map) {
		free(map->array);
		free(map->slot);
		free(map);
	}
}
// 将map注入table
static void
_inject(struct map_kv * table, struct map_ip *map)
{
	if (map->array) {
		int n = 0;
		int i;
		for (i=0;i<(int)map->array_size;i++) {
			if (map->array[i]) {
				table[n].id = i;
				table[n].pointer = map->array[i];
				++ n;
			}
		}
	} else {
		int i;
		for (i=0;i<(int)map->hash_size;i++) {
			table[i].id = map->slot[i].id;
			table[i].pointer = map->slot[i].pointer;
		}
	}
}
// 合并a和b
struct map_ip *
_pbcM_ip_combine(struct map_ip *a, struct map_ip *b)
{
	int sz = (int)(a->hash_size + b->hash_size);
	struct map_kv * table = (struct map_kv *)malloc(sz * sizeof(struct map_kv));
	memset(table , 0 , 	sz * sizeof(struct map_kv));
	_inject(table, a);
	_inject(table + a->hash_size, b);
	struct map_ip * r = _pbcM_ip_new(table, sz);
	free(table);
	return r;
}
// 根据id查询id_pointer map
void *
_pbcM_ip_query(struct map_ip * map, int id)
{
	if (map == NULL)
		return NULL;
	if (map->array) {
		if (id>=0 && id<(int)map->array_size)
			return map->array[id];
		return NULL;
	}
	int hash = (unsigned)id % map->hash_size;
	struct _pbcM_ip_slot * slot = &map->slot[hash];
	for (;;) {
		if (slot->id == id) {
			return slot->pointer;
		}
		if (slot->next == 0) {
			return NULL;
		}
		slot = &map->slot[slot->next-1];
	}
}
// string-pointer槽
struct _pbcM_sp_slot {
	const char *key;     // key
	size_t hash;         // hash
	void *pointer;       // 数据指针
	int next;            // 下一个
};
// 一个字符串映射到指针的动态哈希表（支持扩容，遍历，heap内存分配策略）
struct map_sp {
	size_t cap;                     // 容量
	size_t size;                    // 当前数量
	struct heap *heap;              // 用来申请内存的堆
	struct _pbcM_sp_slot * slot;    // 槽链表
};
// 新建一个string-pointer map
struct map_sp *
_pbcM_sp_new(int max , struct heap *h)
{
	struct map_sp * ret = (struct map_sp *)HMALLOC(sizeof(struct map_sp));
	int cap = 1;
	while (cap < max) {
		cap *=2;
	}
	ret->cap = cap;
	ret->size = 0;
	ret->slot = (struct _pbcM_sp_slot *)HMALLOC(ret->cap * sizeof(struct _pbcM_sp_slot));
	memset(ret->slot,0,sizeof(struct _pbcM_sp_slot) * ret->cap);
	ret->heap = h;
	return ret;
}
// 删除string-pointer map
void
_pbcM_sp_delete(struct map_sp *map)
{
	if (map && map->heap == NULL) {
		_pbcM_free(map->slot);
		_pbcM_free(map);
	}
}

static void _pbcM_sp_rehash(struct map_sp *map);
// 递归地向字符串map插入hash-value对；
static void
_pbcM_sp_insert_hash(struct map_sp *map, const char *key, size_t hash_full, void * value)
{
	if (map->cap > map->size) {
		size_t hash = hash_full & (map->cap-1);
		struct _pbcM_sp_slot * slot = &map->slot[hash];
		if (slot->key == NULL) {
			slot->key = key;
			slot->pointer = value;
			slot->hash = hash_full;
		} else {
			int empty = (hash + 1) & (map->cap-1);
			while(map->slot[empty].key != NULL) {
				empty = (empty + 1) & (map->cap-1);
			}
			struct _pbcM_sp_slot * empty_slot = &map->slot[empty];
			empty_slot->next = slot->next;
			slot->next = empty + 1;
			empty_slot->pointer = value;
			empty_slot->key = key;
			empty_slot->hash = hash_full;
		}
		map->size++;
		return;
	}
	_pbcM_sp_rehash(map);
	_pbcM_sp_insert_hash(map, key, hash_full, value);
}
// 重新hash化
static void
_pbcM_sp_rehash(struct map_sp *map) {
	struct heap * h = map->heap;
	struct _pbcM_sp_slot * old_slot = map->slot;
	size_t size = map->size;
	map->size = 0;
	map->cap *= 2; // 容量翻倍
	map->slot = (struct _pbcM_sp_slot *)HMALLOC(sizeof(struct _pbcM_sp_slot)*map->cap);
	memset(map->slot,0,sizeof(struct _pbcM_sp_slot)*map->cap);
	size_t i;
	for (i=0;i<size;i++) {
		_pbcM_sp_insert_hash(map, old_slot[i].key, old_slot[i].hash, old_slot[i].pointer);
	}
	if (h == NULL) {
		_pbcM_free(old_slot);
	}
}
// 递归查询key并更新hash值
static void **
_pbcM_sp_query_insert_hash(struct map_sp *map, const char *key, size_t hash_full)
{
	size_t hash = hash_full & (map->cap-1);
	struct _pbcM_sp_slot * slot = &map->slot[hash];
	if (slot->key == NULL) {
		if (map->cap <= map->size) 
			goto _rehash;
		slot->key = key;
		slot->hash = hash_full;
		map->size++;
		return &(slot->pointer);
	} else {
		for (;;) {
			if (slot->hash == hash_full && strcmp(slot->key, key) == 0) 
				return &(slot->pointer);
			if (slot->next == 0) {
				break;
			}
			slot = &map->slot[slot->next-1];
		}
		if (map->cap <= map->size) 
			goto _rehash;

		int empty = (hash + 1) & (map->cap-1);
		while(map->slot[empty].key != NULL) {
			empty = (empty + 1) & (map->cap-1);
		}
		struct _pbcM_sp_slot * empty_slot = &map->slot[empty];
		empty_slot->next = slot->next;
		slot->next = empty + 1;
		empty_slot->key = key;
		empty_slot->hash = hash_full;

		map->size++;

		return &(empty_slot->pointer);
	}
_rehash:
	_pbcM_sp_rehash(map);
	return _pbcM_sp_query_insert_hash(map, key, hash_full);
}
// 向类型map插入键值对
void
_pbcM_sp_insert(struct map_sp *map, const char *key, void * value)
{
	_pbcM_sp_insert_hash(map,key,calc_hash(key),value);
}
// 查询并更新hash
void **
_pbcM_sp_query_insert(struct map_sp *map, const char *key)
{
	return _pbcM_sp_query_insert_hash(map,key,calc_hash(key));
}
// 根据key查询"字符串-指针映射map"
void *
_pbcM_sp_query(struct map_sp *map, const char *key)
{
	if (map == NULL)
		return NULL;
	size_t hash_full = calc_hash(key);
	size_t hash = hash_full & (map->cap -1);

	struct _pbcM_sp_slot * slot = &map->slot[hash];
	if (slot->key == NULL)
		return NULL;
	for (;;) {
		if (slot->hash == hash_full && strcmp(slot->key, key) == 0) {
			return slot->pointer;
		}
		if (slot->next == 0) {
			return NULL;
		}
		slot = &map->slot[slot->next-1];
	}
}
// 遍历map执行函数；func:需要执行的函数
void 
_pbcM_sp_foreach(struct map_sp *map, void (*func)(void *p))
{
	size_t i;
	for (i=0;i<map->cap;i++) {
		if (map->slot[i].pointer) {
			func(map->slot[i].pointer);
		}
	}
}
// 遍历map，提供user data来执行函数；func:需要执行的函数，ud:user data
void 
_pbcM_sp_foreach_ud(struct map_sp *map, void (*func)(void *p, void *ud), void *ud)
{
	size_t i;
	for (i=0;i<map->cap;i++) {
		if (map->slot[i].pointer) {
			func(map->slot[i].pointer,ud);
		}
	}
}
// 查找map的第一个有效值
static int
_find_first(struct map_sp *map)
{
	size_t i;
	for (i=0;i<map->cap;i++) {
		if (map->slot[i].pointer) {
			return i;
		}
	}
	return -1;
}
// 查找key的下一个key
static int
_find_next(struct map_sp *map, const char *key)
{
	size_t hash_full = calc_hash(key);
	size_t hash = hash_full & (map->cap -1);

	struct _pbcM_sp_slot * slot = &map->slot[hash];
	if (slot->key == NULL)
		return -1;
	for (;;) {
		if (slot->hash == hash_full && strcmp(slot->key, key) == 0) {
			int i = slot - map->slot + 1;
			while(i<map->cap) {
				if (map->slot[i].pointer) {
					return i;
				}
				++i;
			}
			return -1;
		}
		if (slot->next == 0) {
			return -1;
		}
		slot = &map->slot[slot->next-1];
	}
}
// 查找key的下一个key（如果没找到，返回第一个key）
void * 
_pbcM_sp_next(struct map_sp *map, const char ** key)
{
	if (map == NULL) {
		*key = NULL;
		return NULL;
	}
	int idx;
	if (*key == NULL) {
		idx = _find_first(map);
	} else {
		idx = _find_next(map, *key);
	}
	if (idx < 0) {
		*key = NULL;
		return NULL;
	}
	*key = map->slot[idx].key;
	return map->slot[idx].pointer;
}



