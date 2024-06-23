#include <malloc.h>
#include <string.h>

#include "hmap.h"
#include "utils.h"

// Number of buckets allocated at has map creation.
#define DEFAULT_NUM_BUCKETS 4 

static HashMapEntry_t* createHashMapEntry(const char* key, void* value); 
static void cleanupHashMapEntry(HashMapEntry_t** entry, HashMapElemCleanupFn_t clenaupFn); 
static uint64_t computeHash(const char* key); 
static uint32_t getBucketIndex(HashMap_t* map, const char* key); 
static void hashMapReinsertEntry(HashMap_t* map, HashMapEntry_t* entry); 
static void hashMapResize(HashMap_t* map);  
static uint32_t getResizeTriggerLimit(HashMap_t* map); 

/* External API */

HashMap_t* createHashMap() {
    HashMap_t* map = mallocChk(sizeof(HashMap_t));
    
    *map = (HashMap_t) {
        .buckets = calloc(DEFAULT_NUM_BUCKETS, sizeof(HashMapEntry_t*)),
        .numBuckets = DEFAULT_NUM_BUCKETS, 
        .itemCnt = 0
    };

    return map;
}

HashMap_t* copyHashMap(const HashMap_t* map, HashMapElemCopyFn_t copyFn) {
    if (!map || !copyFn)
        return NULL;
    HashMap_t* newMap = createHashMap();

    HashMapIter_t iter = createHashMapIter(map);
    HashMapEntry_t* entry = hashMapIterGetNext(map, &iter);
    while (entry)  {
        hashMapInsert(newMap, entry->key, copyFn(entry->value));
        entry = hashMapIterGetNext(map, &iter);
    }
    return newMap;
}


void cleanupHashMapElements(HashMap_t* map, HashMapElemCleanupFn_t cleanupFn) {
    if (!map) return;
    HashMapIter_t iter = createHashMapIter(map);
    HashMapEntry_t* entry = hashMapIterGetNext(map, &iter);
    while (entry)  {
        HashMapEntry_t* next = hashMapIterGetNext(map, &iter);
        cleanupHashMapEntry(&entry, cleanupFn);
        entry = next;
    }
}


void cleanupHashMap(HashMap_t** map, HashMapElemCleanupFn_t cleanupFn) {
    if (!(*map)) return;

    cleanupHashMapElements(*map, cleanupFn);
    free((*map)->buckets);
    
    free(*map);
    *map = NULL;
}

HashMapIter_t createHashMapIter(const HashMap_t* map)  {
    for (int32_t i = 0; i < map->numBuckets; i++) {
        if (map->buckets[i]) {
            return (HashMapIter_t) {
                .curBucket = i,
                .curElem = map->buckets[i]
            };
        }
    }
    return (HashMapIter_t) {.curElem=NULL};
}

HashMapEntry_t* hashMapIterGetNext(const HashMap_t* map, HashMapIter_t* iter) {
    if (!iter || !iter->curElem) return NULL;
    
    HashMapEntry_t* ret = iter->curElem;
    if (iter->curElem->next){
        iter->curElem = iter->curElem->next;
        return ret; 
    }
        
    // seek starting at next bucket 
    iter->curElem = NULL;
    iter->curBucket++; 
    while (iter->curBucket < map->numBuckets) {
        if (map->buckets[iter->curBucket]) {
            iter->curElem = map->buckets[iter->curBucket];
            return ret;
        }
        iter->curBucket++;
    }

    return ret;
}



void* hashMapInsert(HashMap_t* map, const char* key, void* value) {  
    uint32_t index = getBucketIndex(map, key);
    void* ret = NULL; // holds previous value in case of key collision.
    if (!map->buckets[index]){
        map->buckets[index] = createHashMapEntry(key, value);
        map->itemCnt++;
    } else {
        HashMapEntry_t* cur = map->buckets[index];
        bool found = false;
        while (!(found = (strcmp(cur->key, key) == 0)) && cur->next) {
            cur = cur->next;
        }
        if (!found) {
            cur->next = createHashMapEntry(key, value);
            map->itemCnt++;
        } else {
            ret = cur->value;
            cur->value = value;
        }
    }

    hashMapResize(map);
    return ret;
}

void* hashMapGet(HashMap_t* map, const char* key) {
    uint32_t index = getBucketIndex(map, key);
    HashMapEntry_t* cur = map->buckets[index];

    while (cur) {
        if (strcmp(cur->key, key) == 0)
            return cur->value;
        cur = cur->next;
    }

    return NULL;
}

static void hashMapResize(HashMap_t* map)  {
    if (map->itemCnt < getResizeTriggerLimit(map)) {
        return;
    }
    
    uint32_t prevSize = map->numBuckets;
    HashMapEntry_t** prevBuckets = map->buckets;

    map->numBuckets = map->numBuckets * 2;  
    map->buckets = (HashMapEntry_t**) calloc(map->numBuckets, sizeof(HashMapEntry_t*));
    if (!map->buckets) HANDLE_OOM();

    for(uint32_t i = 0; i < prevSize; i++) {
        HashMapEntry_t* entry = prevBuckets[i];
        while (entry) {
            HashMapEntry_t* next = entry->next;
            entry->next = NULL;
            hashMapReinsertEntry(map, entry);
            entry = next;
        }
    }

    free(prevBuckets);
}

static void hashMapReinsertEntry(HashMap_t* map, HashMapEntry_t* entry) {
    uint32_t index = getBucketIndex(map, entry->key);
    
    if (!map->buckets[index]){
        map->buckets[index] = entry;
    } else {
        HashMapEntry_t* cur = map->buckets[index];
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = entry;
    }
}


static uint32_t getResizeTriggerLimit(HashMap_t* map) {
    return ((3 * map->numBuckets) / 4); 
}

static uint32_t getBucketIndex(HashMap_t* map, const char* key) {
    uint64_t hash = computeHash(key);
    return (uint32_t)(hash & (uint64_t)(map->numBuckets - 1)); 
}

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static uint64_t computeHash(const char* key) {
    uint64_t hash = FNV_OFFSET;
    while(*key) {
        hash ^= (uint64_t)(unsigned char)(*key);
        hash *= FNV_PRIME;
        key++;
    }
    return hash;
}

static HashMapEntry_t* createHashMapEntry(const char* key, void* value) {
    HashMapEntry_t* entry = (HashMapEntry_t*)malloc(sizeof(HashMapEntry_t));
    if (!entry) HANDLE_OOM();
     
    *entry = (HashMapEntry_t) {
        .key = cloneString(key),
        .value = value,
        .next = NULL
    };

    return entry;
}


static void cleanupHashMapEntry(HashMapEntry_t** entry, HashMapElemCleanupFn_t cleanupFn) {
    if (!(*entry))
        return;
        
    free((*entry)->key);
    if (cleanupFn)
        cleanupFn(&(*entry)->value);
    
    free(*entry);
    *entry = NULL;
}