#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

struct header_t {
    size_t size;
    unsigned isfree;
    struct header_t* next;
};

typedef char ALIGN[16];  // used for identifying unique 16 byte address

union header {
    struct {
        size_t size;
        unsigned isfree;
        union header* next;
    } s;
    ALIGN addy;
};

typedef union header header_t;

header_t *head = NULL, *tail = NULL;
pthread_mutex_t global_malloc_lock;

header_t* get_free_block(size_t size) {
    header_t* curr = head;
    while (curr) {
        if (curr->s.isfree && curr->s.size >= size)
            return curr;
        curr = curr->s.next;
    }
    return NULL;
}

void* malloc(size_t size) {
    if (size == 0) return NULL;

    size_t total_size;
    void* block = NULL;
    header_t* header = NULL;

    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);
    if (header) {
        header->s.isfree = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)((char*)header + sizeof(header_t));  // Correct pointer arithmetic
    }

    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void*)-1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    header = (header_t*)block;
    header->s.size = size;
    header->s.isfree = 0;
    header->s.next = NULL;
    if (!head) head = header;
    if (tail) tail->s.next = header;
    tail = header;

    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)((char*)header + sizeof(header_t));  // Correct pointer arithmetic
}

void free(void* block) {
    if (!block) return;

    pthread_mutex_lock(&global_malloc_lock);

    header_t* header = (header_t*)((char*)block - sizeof(header_t));
    void* programbreak = sbrk(0);

    if ((char*)block + header->s.size == programbreak) {
        if (head == tail) {
            head = tail = NULL;
        } else {
            header_t* tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        sbrk(0 - sizeof(header_t) - header->s.size);
    } else {
        header->s.isfree = 1;
    }

    pthread_mutex_unlock(&global_malloc_lock);
}

void* calloc(size_t num, size_t nsize) {
    size_t size = num * nsize;
    if (num == 0) return NULL;
    if (nsize != (size / num)) return NULL;  // Overflow check
    void* block = malloc(size);
    if (!block) return NULL;
    memset(block, 0, size);  // Initialize memory to zero
    return block;
}

void* realloc(void *block, size_t size) {
    if (!block || size == 0) return malloc(size);  // Allocate new block if block is null or size is 0
    header_t* header = (header_t*)((char*)block - sizeof(header_t));

    if (header->s.size >= size) return block;  // Return the same block if current size is sufficient

    void* ret = malloc(size);
    if (ret) {
        memcpy(ret, block, header->s.size);  // Copy data to new block
        free(block);  // Free old block
    }
    return ret;
}

