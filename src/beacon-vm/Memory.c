#include "beacon-lang/Memory.h"
#include <stdlib.h>

beacon_MemoryHeap_t *beacon_createMemoryHeap(void)
{
    return calloc(1, sizeof(beacon_MemoryHeap_t));
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
    
}
