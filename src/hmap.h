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
    uint32_t numBuckets;
    uint32_t itemCnt;
} HashMap_t;

typedef struct HashMapIter {
    uint32_t curBucket; 
    HashMapEntry_t* curElem; 
} HashMapIter_t;

typedef void (*HashMapElemCleanupFn_t) (void** elem);
typedef void* (*HashMapElemCopyFn_t) (const void* elem);

HashMap_t* createHashMap();
HashMap_t* copyHashMap(const HashMap_t* map, HashMapElemCopyFn_t copyFn);
void cleanupHashMapElements(HashMap_t* map, HashMapElemCleanupFn_t cleanupFn);
void cleanupHashMap(HashMap_t** map, HashMapElemCleanupFn_t cleanupFn);

HashMapIter_t createHashMapIter(const HashMap_t* map);
HashMapEntry_t* hashMapIterGetNext(const HashMap_t* map, HashMapIter_t* iter);

void* hashMapInsert(HashMap_t* map, const char* key , void* value);
void* hashMapGet(HashMap_t* map, const char* key);

#endif 