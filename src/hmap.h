#ifndef _HMAP_H_
#define _HMAP_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct HashMapEntry {
    char* key; // local ownership 
    void* value; // local ownership 
    struct HashMapEntry* next; 
} HashMapEntry_t;

typedef struct HashMap {
    HashMapEntry_t** buckets;
    uint32_t num_buckets;
    uint32_t itemCnt;
} HashMap_t;

typedef struct HashMapIter {
    uint32_t cur_bucket; 
    HashMapEntry_t* cur_elem; 
} HashMapIter_t;

typedef void (*HashMapElemCleanupFn_t) (void** elem);
typedef void* (*HashMapElemCopyFn_t) (const void* elem);

HashMap_t* create_hash_map();
HashMap_t* copy_hash_map(const HashMap_t* map, HashMapElemCopyFn_t copyFn);
void cleanup_hash_map_elements(HashMap_t* map, HashMapElemCleanupFn_t cleanupFn);
void cleanup_hash_map(HashMap_t** map, HashMapElemCleanupFn_t cleanupFn);

HashMapIter_t create_hash_map_iter(const HashMap_t* map);
HashMapEntry_t* hash_map_iter_get_next(const HashMap_t* map, HashMapIter_t* iter);

void* hash_map_insert(HashMap_t* map, const char* key , void* value);
void* hash_map_get(HashMap_t* map, const char* key);

#endif 