#include "beacon-lang/ChunkedAllocator.h"
#include "VirtualMemory.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t beacon_chunkedAllocator_sizeAlignedTo(size_t size, size_t alignment)
{
    return (size + alignment - 1) & (~(alignment - 1));
}

void beacon_chunkedAllocator_initialize(beacon_chunkedAllocator_t *allocator, size_t chunkSize, bool requiresExecutableMapping)
{
    allocator->chunkSize = chunkSize;
    allocator->requiresExecutableMapping = requiresExecutableMapping;
}

void beacon_chunkedAllocator_destroy(beacon_chunkedAllocator_t *allocator)
{
    beacon_chunkedAllocatorChunk_t *chunk = allocator->firstChunk;
    while(chunk)
    {
        beacon_chunkedAllocatorChunk_t *nextChunk = chunk->next;
        size_t fullChunkSize = sizeof(beacon_chunkedAllocatorChunk_t) + chunk->capacity;

        if(allocator->requiresExecutableMapping && chunk->writeableMapping != chunk->executableMapping)
        {
            beacon_chunkedAllocatorChunk_t *writeableMapping = chunk->writeableMapping;
            beacon_chunkedAllocatorChunk_t *executableMapping = chunk->executableMapping;
            beacon_virtualMemory_freeSystemMemoryWithDualMapping(fullChunkSize, chunk->dualMappingHandle, writeableMapping, executableMapping);
            chunk = nextChunk;

        }
        else
        {
            beacon_chunkedAllocatorChunk_t *writeableMapping = chunk->writeableMapping;
            if(writeableMapping)
                beacon_virtualMemory_freeSystemMemory(writeableMapping, fullChunkSize);
        }

        chunk = nextChunk;
    }
}

static beacon_chunkedAllocatorChunk_t *beacon_chunkedAllocator_ensureChunkWithRequiredCapacity(beacon_chunkedAllocator_t *allocator, size_t size, size_t alignment)
{
    beacon_chunkedAllocatorChunk_t **currentChunk = &allocator->currentChunk;

    // Advance the current chunk.
    size_t requiredAlignedSize = beacon_chunkedAllocator_sizeAlignedTo(size, alignment);
    while(*currentChunk && ((*currentChunk)->capacity - beacon_chunkedAllocator_sizeAlignedTo((*currentChunk)->size, alignment)) < requiredAlignedSize)
        *currentChunk = (*currentChunk)->next;

    // Create a new chunk if needed
    if(*currentChunk == NULL)
    {
        beacon_chunkedAllocatorChunk_t *newChunkWriteableMapping = NULL;
        beacon_chunkedAllocatorChunk_t *newChunkExecutableMapping = NULL;

        if(allocator->requiresExecutableMapping)
        {
            if(beacon_virtualMemory_hasSupportForRWXMapping())
            {
                newChunkExecutableMapping = newChunkWriteableMapping = (beacon_chunkedAllocatorChunk_t*)beacon_virtualMemory_allocateSystemMemory(allocator->chunkSize);
                memset(newChunkWriteableMapping, 0, sizeof(beacon_chunkedAllocatorChunk_t));
            }
            else
            {
                void *handle = beacon_virtualMemory_allocateSystemMemoryWithDualMapping(allocator->chunkSize, (void**)&newChunkWriteableMapping, (void**)&newChunkExecutableMapping);
                memset(newChunkWriteableMapping, 0, sizeof(beacon_chunkedAllocatorChunk_t));
                newChunkWriteableMapping->dualMappingHandle = handle;
            }
        }
        else
        {
            newChunkWriteableMapping = (beacon_chunkedAllocatorChunk_t*)beacon_virtualMemory_allocateSystemMemory(allocator->chunkSize);
            memset(newChunkWriteableMapping, 0, sizeof(beacon_chunkedAllocatorChunk_t));
        }
        
        newChunkWriteableMapping->capacity = allocator->chunkSize - sizeof(beacon_chunkedAllocatorChunk_t);
        newChunkWriteableMapping->writeableMapping = newChunkWriteableMapping;
        newChunkWriteableMapping->executableMapping = newChunkExecutableMapping;

        if(!allocator->firstChunk)
            allocator->firstChunk = newChunkWriteableMapping;

        if(allocator->lastChunk)
        {
            if(allocator->requiresExecutableMapping &&
                !beacon_virtualMemory_lockCodePagesForWriting(allocator->lastChunk->writeableMapping, allocator->lastChunk->executableMapping, sizeof(beacon_chunkedAllocatorChunk_t)))
                abort();

            allocator->lastChunk->next = newChunkWriteableMapping;
            newChunkWriteableMapping->previous = allocator->lastChunk;

            if(allocator->requiresExecutableMapping)
                beacon_virtualMemory_unlockCodePagesForExecution(allocator->lastChunk->writeableMapping, allocator->lastChunk->executableMapping, sizeof(beacon_chunkedAllocatorChunk_t));
        }
        allocator->lastChunk = newChunkWriteableMapping;

        *currentChunk = newChunkWriteableMapping;
    }

    return *currentChunk;

}

void* beacon_chunkedAllocator_allocate(beacon_chunkedAllocator_t *allocator, size_t size, size_t alignment)
{
    beacon_chunkedAllocatorChunk_t *chunk = beacon_chunkedAllocator_ensureChunkWithRequiredCapacity(allocator, size, alignment);
    assert(chunk);

    size_t alignedOffset = beacon_chunkedAllocator_sizeAlignedTo(chunk->size, alignment);
    uint8_t *result = (uint8_t*)(chunk + 1) + alignedOffset;
    chunk->size = alignedOffset + size;
    assert(chunk->size <= chunk->capacity);

    return result;
}

void beacon_chunkedAllocator_allocateWithDualMapping(beacon_chunkedAllocator_t *allocator, size_t size, size_t alignment, void **writeableMapping, void **executableMapping)
{

    beacon_chunkedAllocatorChunk_t *chunk = beacon_chunkedAllocator_ensureChunkWithRequiredCapacity(allocator, size, alignment);
    assert(chunk);

    size_t alignedOffset = beacon_chunkedAllocator_sizeAlignedTo(chunk->size, alignment);
    if(writeableMapping)
        *writeableMapping = (uint8_t*)(chunk->writeableMapping + 1) + alignedOffset;
    if(executableMapping)
        *executableMapping = (uint8_t*)(chunk->executableMapping + 1) + alignedOffset;

    if(!beacon_virtualMemory_lockCodePagesForWriting(chunk->writeableMapping, chunk->executableMapping, sizeof(beacon_chunkedAllocatorChunk_t)))
        abort();

    chunk->size = alignedOffset + size;
    assert(chunk->size <= chunk->capacity);

    beacon_virtualMemory_unlockCodePagesForExecution(chunk->writeableMapping, chunk->executableMapping, sizeof(beacon_chunkedAllocatorChunk_t));
}
