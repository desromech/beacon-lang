#ifndef BEACON_MEMORY_H
#define BEACON_MEMORY_H

#pragma once

#include "ObjectModel.h"
#include <stddef.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct beacon_context_s beacon_context_t;

typedef struct beacon_MemoryAllocationHeader_s beacon_MemoryAllocationHeader_t;

typedef struct beacon_MemoryAllocationHeader_s
{
    beacon_MemoryAllocationHeader_t *nextAllocation;
    size_t allocationSize;
} beacon_MemoryAllocationHeader_t;

typedef struct beacon_MemoryHeap_s
{
    beacon_MemoryAllocationHeader_t *lastAllocation;
    uint8_t whiteGCColor;
    uint8_t grayGCColor;
    uint8_t blackGCColor;
    int gcDisableCount;
    size_t allocatedByteCount;
    size_t gcTriggerLimit;
    beacon_context_t *context;

    size_t markingStackCapacity;
    size_t markingStackSize;
    beacon_oop_t *markingStack;
} beacon_MemoryHeap_t;

typedef enum beacon_StackFrameRecordKind_e
{
    StackFrameBytecodeMethodRecord = 0,
} beacon_StackFrameRecordKind_t;

typedef struct beacon_StackFrameRecord_s
{
    struct beacon_StackFrameRecord_s *previousRecord;
    struct beacon_context_s *context;
    beacon_StackFrameRecordKind_t kind;
    union
    {
        struct
        {
            beacon_CompiledCode_t *code;
            beacon_oop_t receiver;
            size_t argumentCount;
            size_t temporaryCount;
            size_t decodedArgumentsTemporaryZoneSize;
            beacon_oop_t *arguments;
            beacon_oop_t *temporaries;
            beacon_oop_t *decodedArgumentsTemporaryZone;

            beacon_oop_t captures;
            beacon_oop_t returnResultValue;
            jmp_buf nonLocalReturnJumpBuffer;
        } bytecodeMethodStackRecord;
    };
} beacon_StackFrameRecord_t;

beacon_StackFrameRecord_t *beacon_getTopStackFrameRecord();
void beacon_pushStackFrameRecord(beacon_StackFrameRecord_t *record);
void beacon_popStackFrameRecord(beacon_StackFrameRecord_t *record);

beacon_MemoryHeap_t *beacon_createMemoryHeap(beacon_context_t *context);
void beacon_destroyMemoryHeap(beacon_MemoryHeap_t *heap);

void beacon_memoryHeapDisableGC(beacon_MemoryHeap_t *heap);
void beacon_memoryHeapEnableGC(beacon_MemoryHeap_t *heap);
void beacon_memoryHeapSafepoint(beacon_context_t *context);

void *beacon_allocateObject(beacon_MemoryHeap_t *heap, size_t size, beacon_ObjectKind_t kind);
void *beacon_allocateObjectWithBehavior(beacon_MemoryHeap_t *heap, beacon_Behavior_t *behavior, size_t size, beacon_ObjectKind_t kind);

#ifdef __cplusplus
}
#endif

#endif //BEACON_MEMORY_H