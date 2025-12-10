#ifndef HASHMAP_H
#define HASHMAP_H

#ifndef HASHMAP_MEMALLOC
#define HASHMAP_MEMALLOC(size) malloc(size)
#endif

#ifndef HASHMAP_MEMFREE
#define HASHMAP_MEMFREE(size) free(size)
#endif

#include <stdint.h>
#include <stddef.h>


struct hashmap_pair_t {
    bool  used;
    int   key;
    void* ptr;
 
    // If the memory size is 0 and 'used' is set to 'true'
    // Then only a pointer was copied here.
    //
    // If the memory size is > 0 and 'used' is set to 'true'
    // Then memory was allocated for the element.
    size_t mem_size;
};

struct hashmap_bucket_t {
    struct hashmap_pair_t* pairs;
    size_t                 pairs_mem_size;
    size_t                 num_pairs;

    // Points to "next" bucket which num_pairs > 0,
    // Note: the elements will be in reverse order from how you added them.
    struct hashmap_bucket_t* next;
};

struct hashmap_t {
    size_t map_size;
    size_t buckets_mem_size;
    struct hashmap_bucket_t* buckets;

    struct hashmap_bucket_t* buckets_link_tail;
};

const char* hashmap_get_errmsg();

struct hashmap_t create_hashmap(size_t initial_map_size);
void             free_hashmap(struct hashmap_t* map);


bool hashmap_key_exists(struct hashmap_t* map, int key,
        struct hashmap_bucket_t** bucket_out,            // Optional
        size_t*                   bucket_pair_index_out  // Optional
);     


uint64_t strtokey(const char* str);


void hashmap_clear(struct hashmap_t* map);

// Add existing pointer to hashmap.
// 'ptr' is only copied (not memory where it points to).
//
// 'false' is returned if map contains the key same already or a memory error happened. 
// 'true' is returned on success.
bool hashmap_add(struct hashmap_t* map, int key, void* ptr);

// Insert or replace existing pointer to hashmap.
// 'ptr' is only copied (not memory where it points to).
//
// 'false' is returned if map doesnt contain the key or a memory error happened. 
// 'true' is returned on success.
bool hashmap_put(struct hashmap_t* map, int key, void* ptr);


// These functions behaviour is same as above but only differences are
// that they allocate memory
// and then copy 'size' number of bytes from 'data_ptr'.
//
// Their memory is freed when:
// 'hashmap_del()' function is called with the corresponding key.
// or 'free_hashmap()' function is called.
bool hashmap_add_new(struct hashmap_t* map, int key, void* data_ptr, size_t size);
bool hashmap_put_new(struct hashmap_t* map, int key, void* data_ptr, size_t size);


// Delete element from hashmap.
//
// 'false' is returned if map doesnt contain the key or memory error happened.
// 'true' is returned on success.
bool hashmap_del(struct hashmap_t* map, int key);



struct hashmap_pair_t* hashmap_get(struct hashmap_t* map, int key);



#endif
