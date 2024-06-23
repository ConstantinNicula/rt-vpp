#include "hmap.h"
#include "log_utils.h"

#include <malloc.h>
#include <string.h>
#include <stdbool.h>

// Number of buckets allocated at has map creation.
#define DEFAULT_NUM_BUCKETS 4 

static HashMapEntry_t* create_hash_map_entry(const char* key, void* value); 
static void cleanup_hash_map_entry(HashMapEntry_t** entry, HashMapElemCleanupFn_t clenaupFn); 
static uint64_t compute_hash(const char* key); 
static uint32_t get_bucket_index(HashMap_t* map, const char* key); 
static void hash_map_reinsert_entry(HashMap_t* map, HashMapEntry_t* entry); 
static void hash_map_resize(HashMap_t* map);  
static uint32_t get_resize_trigger_limit(HashMap_t* map); 

/* External API */

HashMap_t* create_hash_map() {
    HashMap_t* map = malloc(sizeof(HashMap_t));
    CHECK(map != NULL, "Could not allocate space", NULL); 

    *map = (HashMap_t) {
        .buckets = calloc(DEFAULT_NUM_BUCKETS, sizeof(HashMapEntry_t*)),
        .num_buckets = DEFAULT_NUM_BUCKETS, 
        .itemCnt = 0
    };

    return map;
}

HashMap_t* copy_hash_map(const HashMap_t* map, HashMapElemCopyFn_t copyFn) {
    if (!map || !copyFn)
        return NULL;
    HashMap_t* newMap = create_hash_map();

    HashMapIter_t iter = create_hash_map_iter(map);
    HashMapEntry_t* entry = hash_map_iter_get_next(map, &iter);
    while (entry)  {
        hash_map_insert(newMap, entry->key, copyFn(entry->value));
        entry = hash_map_iter_get_next(map, &iter);
    }
    return newMap;
}


void cleanup_hash_map_elements(HashMap_t* map, HashMapElemCleanupFn_t cleanupFn) {
    if (!map) return;
    HashMapIter_t iter = create_hash_map_iter(map);
    HashMapEntry_t* entry = hash_map_iter_get_next(map, &iter);
    while (entry)  {
        HashMapEntry_t* next = hash_map_iter_get_next(map, &iter);
        cleanup_hash_map_entry(&entry, cleanupFn);
        entry = next;
    }
}


void cleanup_hash_map(HashMap_t** map, HashMapElemCleanupFn_t cleanupFn) {
    if (!(*map)) return;

    cleanup_hash_map_elements(*map, cleanupFn);
    free((*map)->buckets);
    
    free(*map);
    *map = NULL;
}

HashMapIter_t create_hash_map_iter(const HashMap_t* map)  {
    for (int32_t i = 0; i < map->num_buckets; i++) {
        if (map->buckets[i]) {
            return (HashMapIter_t) {
                .cur_bucket = i,
                .cur_elem = map->buckets[i]
            };
        }
    }
    return (HashMapIter_t) {.cur_elem=NULL};
}

HashMapEntry_t* hash_map_iter_get_next(const HashMap_t* map, HashMapIter_t* iter) {
    if (!iter || !iter->cur_elem) return NULL;
    
    HashMapEntry_t* ret = iter->cur_elem;
    if (iter->cur_elem->next){
        iter->cur_elem = iter->cur_elem->next;
        return ret; 
    }
        
    // seek starting at next bucket 
    iter->cur_elem = NULL;
    iter->cur_bucket++; 
    while (iter->cur_bucket < map->num_buckets) {
        if (map->buckets[iter->cur_bucket]) {
            iter->cur_elem = map->buckets[iter->cur_bucket];
            return ret;
        }
        iter->cur_bucket++;
    }

    return ret;
}



void* hash_map_insert(HashMap_t* map, const char* key, void* value) {  
    uint32_t index = get_bucket_index(map, key);
    void* ret = NULL; // holds previous value in case of key collision.
    if (!map->buckets[index]){
        map->buckets[index] = create_hash_map_entry(key, value);
        map->itemCnt++;
    } else {
        HashMapEntry_t* cur = map->buckets[index];
        bool found = false;
        while (!(found = (strcmp(cur->key, key) == 0)) && cur->next) {
            cur = cur->next;
        }
        if (!found) {
            cur->next = create_hash_map_entry(key, value);
            map->itemCnt++;
        } else {
            ret = cur->value;
            cur->value = value;
        }
    }

    hash_map_resize(map);
    return ret;
}

void* hash_map_get(HashMap_t* map, const char* key) {
    uint32_t index = get_bucket_index(map, key);
    HashMapEntry_t* cur = map->buckets[index];

    while (cur) {
        if (strcmp(cur->key, key) == 0)
            return cur->value;
        cur = cur->next;
    }

    return NULL;
}

static void hash_map_resize(HashMap_t* map)  {
    if (map->itemCnt < get_resize_trigger_limit(map)) {
        return;
    }
    
    uint32_t prevSize = map->num_buckets;
    HashMapEntry_t** prevBuckets = map->buckets;

    uint32_t new_num_buckets = map->num_buckets * 2;
    HashMapEntry_t** new_buckets = calloc(map->num_buckets, sizeof(HashMapEntry_t*));
    if (!new_buckets) {
        ERROR("Calloc failed \n");
        return; /* Do not perform resize*/
    }

    map->num_buckets = new_num_buckets;  
    map->buckets = new_buckets;

    for(uint32_t i = 0; i < prevSize; i++) {
        HashMapEntry_t* entry = prevBuckets[i];
        while (entry) {
            HashMapEntry_t* next = entry->next;
            entry->next = NULL;
            hash_map_reinsert_entry(map, entry);
            entry = next;
        }
    }

    free(prevBuckets);
}

static void hash_map_reinsert_entry(HashMap_t* map, HashMapEntry_t* entry) {
    uint32_t index = get_bucket_index(map, entry->key);
    
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


static uint32_t get_resize_trigger_limit(HashMap_t* map) {
    return ((3 * map->num_buckets) / 4); 
}

static uint32_t get_bucket_index(HashMap_t* map, const char* key) {
    uint64_t hash = compute_hash(key);
    return (uint32_t)(hash & (uint64_t)(map->num_buckets - 1)); 
}

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static uint64_t compute_hash(const char* key) {
    uint64_t hash = FNV_OFFSET;
    while(*key) {
        hash ^= (uint64_t)(unsigned char)(*key);
        hash *= FNV_PRIME;
        key++;
    }
    return hash;
}

static HashMapEntry_t* create_hash_map_entry(const char* key, void* value) {
    HashMapEntry_t* entry = (HashMapEntry_t*)malloc(sizeof(HashMapEntry_t));
    CHECK(entry != NULL, "Malloc failed", NULL);
    
    *entry = (HashMapEntry_t) {
        .key = strdup(key),
        .value = value,
        .next = NULL
    };

    return entry;
}


static void cleanup_hash_map_entry(HashMapEntry_t** entry, HashMapElemCleanupFn_t cleanupFn) {
    if (!(*entry))
        return;
        
    free((*entry)->key);
    if (cleanupFn)
        cleanupFn(&(*entry)->value);
    
    free(*entry);
    *entry = NULL;
}