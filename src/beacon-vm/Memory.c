#include "beacon-lang/Memory.h"
#include <stdlib.h>

beacon_MemoryHeap_t *beacon_createMemoryHeap(void)
{
    beacon_MemoryHeap_t *heap = calloc(1, sizeof(beacon_MemoryHeap_t));
    heap->whiteGCColor = 0;
    heap->grayGCColor = 1;
    heap->blackGCColor = 2;
    return heap;
}

void beacon_destroyMemoryHeap(beacon_MemoryHeap_t *heap)
{
    beacon_MemoryAllocationHeader_t *position = heap->lastAllocation;
    while(position)
    {
        beacon_MemoryAllocationHeader_t *nextPosition = position->nextAllocation;
        free(position);
        position = nextPosition;
    }
}

beacon_ObjectHeader_t *beacon_allocateObject(beacon_MemoryHeap_t *heap, size_t size, beacon_ObjectKind_t kind)
{
    size_t allocationSize = sizeof(beacon_MemoryAllocationHeader_t) + size;
    beacon_MemoryAllocationHeader_t *allocation = calloc(1, allocationSize);
    allocation->nextAllocation = heap->lastAllocation;
    heap->lastAllocation = allocation;

    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t *)(allocation + 1);
    header->objectKind = kind;
    header->gcColor = heap->whiteGCColor;

    return header;
}
