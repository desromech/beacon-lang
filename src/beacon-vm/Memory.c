#include "beacon-lang/Memory.h"
//#include <stdlib.h>
#include <assert.h>

beacon_MemoryHeap_t *beacon_createMemoryHeap(beacon_context_t *context)
{
    beacon_MemoryHeap_t *heap = calloc(1, sizeof(beacon_MemoryHeap_t));
    heap->whiteGCColor = 0;
    heap->grayGCColor = 1;
    heap->blackGCColor = 2;
    heap->gcDisableCount = 0;
    heap->context = context;
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

void beacon_memoryHeapDisableGC(beacon_MemoryHeap_t *heap)
{
    ++heap->gcDisableCount;
}

void beacon_memoryHeapEnableGC(beacon_MemoryHeap_t *heap)
{
    assert(heap->gcDisableCount > 0);
    --heap->gcDisableCount;
}

void beacon_memoryHeapSafepoint(beacon_MemoryHeap_t *heap)
{
    if(heap->gcDisableCount > 0)
        return;

    // TODO: Check the GC activation policy.

}

void *beacon_allocateObject(beacon_MemoryHeap_t *heap, size_t size, beacon_ObjectKind_t kind)
{
    return beacon_allocateObjectWithBehavior(heap, NULL, size, kind);
}

void *beacon_allocateObjectWithBehavior(beacon_MemoryHeap_t *heap, beacon_Behavior_t *behavior, size_t size, beacon_ObjectKind_t kind)
{
    assert(size >= sizeof(beacon_ObjectHeader_t));
    size_t allocationSize = sizeof(beacon_MemoryAllocationHeader_t) + size;
    beacon_MemoryAllocationHeader_t *allocation = calloc(1, allocationSize);
    allocation->nextAllocation = heap->lastAllocation;
    heap->lastAllocation = allocation;

    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t *)(allocation + 1);
    header->behavior = behavior;
    header->objectKind = kind;
    header->gcColor = heap->whiteGCColor;
    header->slotCount = size - sizeof(beacon_ObjectHeader_t);
    if(kind != BeaconObjectKindBytes)
        header->slotCount /= sizeof(beacon_oop_t);

    return header;
}