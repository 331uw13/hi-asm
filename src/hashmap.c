#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "hashmap.h"


static char HASHMAP_ERRMSG[512] = { 0 };


static inline uint64_t next_pow2_64(uint64_t i) {
    i--;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i |= i >> 32;
    i++;
    return i;
}

const char* hashmap_get_errmsg() {
    return HASHMAP_ERRMSG;
}

static void hashmap_error_ext(const char* from_func, const char* fmt, ...) {
    va_list args;
    va_start(args);
    memset(HASHMAP_ERRMSG, 0, sizeof(HASHMAP_ERRMSG));
    
    const size_t curr_size 
        = snprintf(HASHMAP_ERRMSG, sizeof(HASHMAP_ERRMSG)-1, 
                "%s(): ", from_func);
    
    vsnprintf(HASHMAP_ERRMSG + curr_size, sizeof(HASHMAP_ERRMSG)-1, fmt, args);

    va_end(args);
}

#define hashmap_error(message, ...)\
    hashmap_error_ext(__func__, message, ##__VA_ARGS__)


// Allocates more memory for bucket if needed.
static bool hashmap_memcheck_bucket(struct hashmap_bucket_t* bucket, size_t new_mem_size) {
    if(bucket->pairs && (bucket->pairs_mem_size >= new_mem_size)) {
        return true; // Nothing needs to be done.
    }
    if(bucket->pairs && (new_mem_size < bucket->pairs_mem_size)) {
        return true; // Dont allow memory size to be decreased.
    }

    struct hashmap_pair_t* new_ptr = realloc(bucket->pairs, new_mem_size);
    if(!new_ptr) {
        hashmap_error("realloc() failed! bucket->pairs = %p, new_mem_size = %li | %s",
                bucket->pairs, new_mem_size, strerror(errno));
        return false;
    }

    const size_t start_idx = bucket->pairs_mem_size / sizeof(*bucket->pairs);
    const size_t end_idx = new_mem_size / sizeof(*bucket->pairs);
   
    bucket->pairs = new_ptr;
    
    for(size_t i = start_idx; i < end_idx; i++) {
        struct hashmap_pair_t* pair = &bucket->pairs[i];
        pair->used = false;
        pair->key = 0;
        pair->ptr = NULL;
        pair->mem_size = 0;
    }

    bucket->pairs_mem_size = new_mem_size;

    return true;
}

// Allocates more memory for hash map bucket array if needed.
static bool hashmap_memcheck_add(struct hashmap_t* map, size_t new_mem_size) {
    if(map->buckets && (map->buckets_mem_size >= new_mem_size)) {
        return true; // Nothing needs to be done.
    }
    if(map->buckets && (new_mem_size < map->buckets_mem_size)) {
        return true; // Dont allow memory size to be decreased.
    }
    
    const size_t new_map_size = next_pow2_64(new_mem_size / sizeof(*map->buckets));

    struct hashmap_bucket_t* new_ptr = realloc(map->buckets, new_map_size * sizeof(*map->buckets));
    if(!new_ptr) {
        hashmap_error("realloc() failed! map->buckets = %p, new_mem_size = %li | %s",
                map->buckets, new_mem_size, strerror(errno));
        return false;
    }

    map->buckets = new_ptr;
    map->buckets_mem_size = new_mem_size;

    for(size_t i = map->map_size; i < new_map_size; i++) {
        struct hashmap_bucket_t* bucket = &map->buckets[i];
        bucket->pairs = NULL;
        bucket->pairs_mem_size = 0;
        bucket->num_pairs = 0;
        bucket->next = NULL;

        // Allocate one element for pair array
        // so in best case scenario we
        // dont need to allocate more memory when adding pairs to hashmap
        hashmap_memcheck_bucket(bucket, sizeof(*bucket->pairs));
    }
    
    map->map_size = new_map_size;

    return true;
}

struct hashmap_t create_hashmap(size_t initial_map_size) {
    struct hashmap_t map;
   
    map.map_size = 0;
    map.buckets_mem_size = 0;
    map.buckets = NULL;
    
    hashmap_memcheck_add(&map, initial_map_size * sizeof(*map.buckets));

    map.buckets_link_tail = NULL;
    return map;
}

void free_hashmap(struct hashmap_t* map) {
    size_t free_count = 0;
    size_t free_count_pairptr = 0;
    for(size_t i = 0; i < map->map_size; i++) {
        struct hashmap_bucket_t* bucket = &map->buckets[i];
        if(bucket->pairs) {
            for(size_t j = 0; j < bucket->num_pairs; j++) {
                struct hashmap_pair_t* pair = &bucket->pairs[j];
                if(pair->ptr && pair->mem_size) {
                    free(pair->ptr);
                    pair->ptr = NULL;
                    free_count_pairptr++;
                }
            }

            free(bucket->pairs);
            free_count++;
        }
            
        bucket->pairs = NULL;
        bucket->pairs_mem_size = 0;
        bucket->num_pairs = 0;
        bucket->next = NULL;
    }

    free(map->buckets);
    map->buckets = NULL;
    map->buckets_link_tail = NULL;
}


uint64_t hash_i32_key(int key) {
    return (key ^ (key & 0xFF000000)) * 2654435761;
}

uint64_t strtokey(const char* str) {
    uint64_t key = 0;

    size_t len = strlen(str);
    for(size_t i = 0; i < len; i++) {
        key = hash_i32_key((int)str[i]);
    }
    return key;
}

void hashmap_clear(struct hashmap_t* map) {
    struct hashmap_bucket_t* bucket = map->buckets_link_tail;
    while(bucket) {
        bucket->num_pairs = 0;
        bucket = bucket->next;
    }
}

bool hashmap_key_exists(struct hashmap_t* map, int key, 
        struct hashmap_bucket_t** bucket_out, size_t* bucket_pair_index_out) {
    
    const size_t bucket_idx = hash_i32_key(key) % map->map_size;
    struct hashmap_bucket_t* bucket = &map->buckets[bucket_idx];
    
    if(bucket_out) {
        *bucket_out = bucket;
    }
   
    for(size_t i = 0; i < bucket->num_pairs; i++) {
        if(bucket->pairs[i].used && (bucket->pairs[i].key == key)) {
            if(bucket_pair_index_out) {
                *bucket_pair_index_out = i;
            }
            return true;
        }
    }

    return false;
}


static inline bool prep_hashmap_op_add(struct hashmap_t* map, int key, 
        struct hashmap_bucket_t** bucket_out, struct hashmap_pair_t** pair_out) {

    const size_t bucket_idx = hash_i32_key(key) % map->map_size;
    struct hashmap_bucket_t* bucket = &map->buckets[bucket_idx];

    if(hashmap_key_exists(map, key, NULL, NULL)) {
        return false;
    }

    // Allocate more memory if needed.
    if(!hashmap_memcheck_bucket(bucket, bucket->pairs_mem_size + sizeof(*bucket->pairs))) {
        return false;
    }

    *bucket_out = bucket;
    *pair_out = &bucket->pairs[bucket->num_pairs];

    return true;
}

static inline bool prep_hashmap_op_put(struct hashmap_t* map, int key, 
        struct hashmap_bucket_t** bucket_out, struct hashmap_pair_t** pair_out, bool* key_exists) {
   
    size_t pair_index = 0;

    *key_exists = hashmap_key_exists(map, key, bucket_out, &pair_index);
    if(!*key_exists) {
    
        // Allocate more memory if needed.
        if(!hashmap_memcheck_bucket
            (*bucket_out, (*bucket_out)->pairs_mem_size + sizeof((*bucket_out)->pairs))) {
            return false;
        }

        *pair_out = &(*bucket_out)->pairs[(*bucket_out)->num_pairs];
    }
    else {
        *pair_out = &(*bucket_out)->pairs[pair_index];
    }

    return true;
}

static inline void hashmap_update_linked_list(struct hashmap_t* map, struct hashmap_bucket_t* bucket) {
    bucket->num_pairs++;
    bucket->next = map->buckets_link_tail;
    map->buckets_link_tail = bucket;
}


bool hashmap_add(struct hashmap_t* map, int key, void* ptr) {
    struct hashmap_bucket_t* bucket = NULL;
    struct hashmap_pair_t* pair = NULL;
    
    if(!prep_hashmap_op_add(map, key, &bucket, &pair)) {
        return false;
    }

    pair->key = key;
    pair->ptr = ptr;
    pair->mem_size = 0;
    pair->used = true;

    hashmap_update_linked_list(map, bucket);
    return true;
}

bool hashmap_add_new(struct hashmap_t* map, int key, void* data_ptr, size_t size) {
    struct hashmap_bucket_t* bucket = NULL;
    struct hashmap_pair_t* pair = NULL;

    if(!prep_hashmap_op_add(map, key, &bucket, &pair)) {
        return false;
    }

    pair->key = key;
    pair->used = true;
    
    pair->ptr = malloc(size);
    pair->mem_size = size;

    memmove(pair->ptr, data_ptr, size);

    hashmap_update_linked_list(map, bucket);
    return true;
}

bool hashmap_put_new(struct hashmap_t* map, int key, void* data_ptr, size_t size) {
    struct hashmap_bucket_t* bucket = NULL;
    struct hashmap_pair_t* pair = NULL;

    bool key_exists = false;
    if(!prep_hashmap_op_put(map, key, &bucket, &pair, &key_exists)) {
        return false;
    }

    pair->key = key;
    pair->used = true;
    
    // Here the 'ptr' may be already allocated because this function can replace it.
    // But dont bother to reallocate it if the memory size seems to be same.

    if(pair->mem_size != size) {
        free(pair->ptr);
        pair->ptr = NULL;
    }

    if(!pair->ptr) {
        pair->ptr = malloc(size);
    }

    memmove(pair->ptr, data_ptr, size);
    pair->mem_size = size;

    if(!key_exists) {
        // New element was added.
        hashmap_update_linked_list(map, bucket);
    }
    return true;
}

bool hashmap_put(struct hashmap_t* map, int key, void* ptr) {
    struct hashmap_bucket_t* bucket = NULL;
    struct hashmap_pair_t* pair = NULL;

    bool key_exists = false;
    if(!prep_hashmap_op_put(map, key, &bucket, &pair, &key_exists)) {
        return false;
    }

    pair->key = key;
    pair->ptr = ptr;
    pair->mem_size = 0;
    pair->used = true;

    if(!key_exists) {
        // New element was added.
        hashmap_update_linked_list(map, bucket);
    }
    return true;
}

bool hashmap_del(struct hashmap_t* map, int key) {
    struct hashmap_bucket_t* bucket = NULL;
    size_t pair_index = 0;

    const bool key_exists = hashmap_key_exists(map, key, &bucket, &pair_index);

    if(key_exists) {
        struct hashmap_pair_t* pair = &bucket->pairs[pair_index];
        pair->used = false;
        pair->key = 0;

        if(pair->ptr && pair->mem_size) {
            free(pair->ptr);
        }

        pair->mem_size = 0;
        pair->ptr = NULL;

        //                 ,-----------.
        //                /             v
        // ... -> [o] -> [o]    [X]    [o] -> ...

        struct hashmap_bucket_t* bucket_it = map->buckets_link_tail;
        struct hashmap_bucket_t* prev_bucket_it = NULL;

        while(bucket_it) {

            if(bucket_it == bucket) {
                if(prev_bucket_it) {
                    prev_bucket_it->next = bucket->next;
                }
                else {
                    // Last bucket doesnt have previous.
                    map->buckets_link_tail = bucket->next;
                }
                break;
            }

            prev_bucket_it = bucket_it;
            bucket_it = bucket_it->next;
        }

        bucket->num_pairs--;
    }

    return key_exists;
}

struct hashmap_pair_t* hashmap_get(struct hashmap_t* map, int key) {
    struct hashmap_pair_t* pair = NULL;

    const size_t bucket_idx = hash_i32_key(key) % map->map_size;
    struct hashmap_bucket_t* bucket = &map->buckets[bucket_idx];

    for(size_t i = 0; i < bucket->num_pairs; i++) {
        struct hashmap_pair_t* pair_it = &bucket->pairs[i];
        if(pair_it->used && (pair_it->key == key)) {
            pair = pair_it;
            break;
        }
    }

    return pair;
}


