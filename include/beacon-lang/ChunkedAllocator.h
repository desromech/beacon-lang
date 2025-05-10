#ifndef BEACON_CHUNKED_ALLOCATOR_H
#define BEACON_CHUNKED_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BEACON_CHUNKED_ALLOCATOR_DEFAULT_CHUNK_SIZE ((size_t)(2<<20))

typedef struct beacon_chunkedAllocatorChunk_s
{
    struct beacon_chunkedAllocatorChunk_s *previous;
    struct beacon_chunkedAllocatorChunk_s *next;

    struct beacon_chunkedAllocatorChunk_s *writeableMapping;
    struct beacon_chunkedAllocatorChunk_s *executableMapping;

    void *dualMappingHandle;
    void *reserved;

    size_t capacity;
    size_t size;
} beacon_chunkedAllocatorChunk_t;

typedef struct beacon_chunkedAllocator_s
{
    beacon_chunkedAllocatorChunk_t* firstChunk;
    beacon_chunkedAllocatorChunk_t* lastChunk;
    beacon_chunkedAllocatorChunk_t* currentChunk;
    size_t chunkSize;
    bool requiresExecutableMapping;
} beacon_chunkedAllocator_t;

typedef struct beacon_chunkedAllocatorIterator_s
{
    beacon_chunkedAllocatorChunk_t* chunk;
    size_t size;
    uint8_t *data;
} beacon_chunkedAllocatorIterator_t;

static inline void beacon_chunkedAllocatorIterator_setForChunk(beacon_chunkedAllocatorIterator_t *iterator, beacon_chunkedAllocatorChunk_t* chunk)
{
    iterator->chunk = chunk;
    if(chunk)
    {
        iterator->size = chunk->size;
        iterator->data = (uint8_t*)(chunk + 1);
    }
    else
    {
        iterator->size = 0;
        iterator->data = NULL;
    }
}

static inline void beacon_chunkedAllocatorIterator_begin(beacon_chunkedAllocator_t *allocator, beacon_chunkedAllocatorIterator_t *iterator)
{
    beacon_chunkedAllocatorIterator_setForChunk(iterator, allocator->firstChunk);
}

static inline void beacon_chunkedAllocatorIterator_advance(beacon_chunkedAllocatorIterator_t *iterator)
{
    if(iterator->chunk)
        beacon_chunkedAllocatorIterator_setForChunk(iterator, iterator->chunk->next);
}

static inline bool beacon_chunkedAllocatorIterator_isValid(beacon_chunkedAllocatorIterator_t *iterator)
{
    return iterator->chunk != NULL;
}

void beacon_chunkedAllocator_initialize(beacon_chunkedAllocator_t *allocator, size_t chunkSize, bool requiresExecutableMapping);
void beacon_chunkedAllocator_destroy(beacon_chunkedAllocator_t *allocator);

void* beacon_chunkedAllocator_allocate(beacon_chunkedAllocator_t *allocator, size_t size, size_t alignment);
void beacon_chunkedAllocator_allocateWithDualMapping(beacon_chunkedAllocator_t *allocator, size_t size, size_t alignment, void **writeableMapping, void **executableMapping);

#endif //BEACON_CHUNKED_ALLOCATOR_H
