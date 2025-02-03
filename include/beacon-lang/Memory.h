#ifndef BEACON_MEMORY_H
#define BEACON_MEMORY_H

#include "ObjectModel.h"
#include <stddef.h>

typedef struct beacon_MemoryAllocationHeader_s beacon_MemoryAllocationHeader_t;

typedef struct beacon_MemoryAllocationHeader_s
{
    beacon_MemoryAllocationHeader_t *nextAllocation;
} beacon_MemoryAllocationHeader_t;

typedef struct beacon_MemoryHeap_s
{
    beacon_MemoryAllocationHeader_t *lastAllocation;
    uint8_t whiteGCColor;
    uint8_t grayGCColor;
    uint8_t blackGCColor;
} beacon_MemoryHeap_t;

beacon_MemoryHeap_t *beacon_createMemoryHeap(void);
void beacon_destroyMemoryHeap(beacon_MemoryHeap_t *heap);

beacon_ObjectHeader_t *beacon_allocateObject(beacon_MemoryHeap_t *heap, size_t size, beacon_ObjectKind_t kind);

#endif //BEACON_MEMORY_H