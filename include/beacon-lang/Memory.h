#ifndef BEACON_MEMORY_H
#define BEACON_MEMORY_H

#pragma once

#include "ObjectModel.h"
#include <stddef.h>

typedef struct beacon_context_s beacon_context_t;

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
    int gcDisableCount;
    beacon_context_t *context;
} beacon_MemoryHeap_t;

beacon_MemoryHeap_t *beacon_createMemoryHeap(beacon_context_t *context);
void beacon_destroyMemoryHeap(beacon_MemoryHeap_t *heap);

void beacon_memoryHeapDisableGC(beacon_MemoryHeap_t *heap);
void beacon_memoryHeapEnableGC(beacon_MemoryHeap_t *heap);
void beacon_memoryHeapSafepoint(beacon_MemoryHeap_t *heap);

void *beacon_allocateObject(beacon_MemoryHeap_t *heap, size_t size, beacon_ObjectKind_t kind);
void *beacon_allocateObjectWithBehavior(beacon_MemoryHeap_t *heap, beacon_Behavior_t *behavior, size_t size, beacon_ObjectKind_t kind);
#endif //BEACON_MEMORY_H