#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
//#include <stdlib.h>
#include <assert.h>

_Thread_local beacon_StackFrameRecord_t *beaconCurrentTopStackFrameRecord = 0;

beacon_StackFrameRecord_t *beacon_getTopStackFrameRecord()
{
    return beaconCurrentTopStackFrameRecord;
}

void beacon_pushStackFrameRecord(beacon_StackFrameRecord_t *record)
{
    record->previousRecord = beaconCurrentTopStackFrameRecord;
    beaconCurrentTopStackFrameRecord = record;
    beacon_memoryHeapSafepoint(record->context);
}

void beacon_popStackFrameRecord(beacon_StackFrameRecord_t *record)
{
    beacon_memoryHeapSafepoint(record->context);
    beaconCurrentTopStackFrameRecord = record->previousRecord;
}

beacon_MemoryHeap_t *beacon_createMemoryHeap(beacon_context_t *context)
{
    beacon_MemoryHeap_t *heap = calloc(1, sizeof(beacon_MemoryHeap_t));
    heap->whiteGCColor = 0;
    heap->grayGCColor = 1;
    heap->blackGCColor = 2;
    heap->gcDisableCount = 0;
    heap->context = context;
    heap->gcTriggerLimit = 1024*4; // Start with a page size trigger limit
    heap->markingStackCapacity = 512;
    heap->markingStackSize = 0;
    heap->markingStack = calloc(heap->markingStackCapacity, sizeof(beacon_oop_t));
    return heap;
}

void beacon_heap_pushReachableObject(beacon_MemoryHeap_t *heap, beacon_oop_t object)
{
    if (beacon_isImmediate(object) || !object)
        return;

    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t *)object;
    if(header->gcColor != heap->whiteGCColor)
        return;

    // Mark gray    
    header->gcColor = heap->grayGCColor;

    // Ensure that we have enough capacity.
    if (heap->markingStackSize == heap->markingStackCapacity)
    {
        size_t newMarkingCapacity = heap->markingStackCapacity * 2;
        beacon_oop_t *newStorage = calloc(newMarkingCapacity, sizeof(beacon_oop_t));
        for(size_t i = 0; i < heap->markingStackSize; ++i)
            newStorage[i] = heap->markingStack[i];

        free(heap->markingStack);
        heap->markingStackCapacity = newMarkingCapacity;
        heap->markingStack = newStorage;

    }

    heap->markingStack[heap->markingStackSize++] = object;
}

void beacon_garbageCollect_markRootsPhase(beacon_context_t *context)
{
    // Mark the classes.
    {
        beacon_oop_t *classes = (beacon_oop_t *)&context->classes;
        size_t classCount = sizeof(context->classes) / sizeof(beacon_oop_t);
        for(size_t i = 0; i < classCount; ++i)
            beacon_heap_pushReachableObject(context->heap, classes[i]);
    }

    // Mark the context roots
    {
        beacon_oop_t *contextRoots = (beacon_oop_t *)&context->roots;
        size_t contextRootCount = sizeof(context->roots) / sizeof(beacon_oop_t);
        for(size_t i = 0; i < contextRootCount; ++i)
            beacon_heap_pushReachableObject(context->heap, contextRoots[i]);
    }

    // Mark the stack
    {
        beacon_StackFrameRecord_t *currentStackRecord = beacon_getTopStackFrameRecord();
        while(currentStackRecord)
        {
            switch (currentStackRecord->kind)
            {
            case StackFrameBytecodeMethodRecord:
            {
                beacon_heap_pushReachableObject(context->heap, (beacon_oop_t)currentStackRecord->bytecodeMethodStackRecord.code);
                for(size_t i = 0; i < currentStackRecord->bytecodeMethodStackRecord.argumentCount; ++i)
                    beacon_heap_pushReachableObject(context->heap, currentStackRecord->bytecodeMethodStackRecord.arguments[i]);
                for(size_t i = 0; i < currentStackRecord->bytecodeMethodStackRecord.temporaryCount; ++i)
                    beacon_heap_pushReachableObject(context->heap, currentStackRecord->bytecodeMethodStackRecord.temporaries[i]);
                for(size_t i = 0; i < currentStackRecord->bytecodeMethodStackRecord.decodedArgumentsTemporaryZoneSize; ++i)
                    beacon_heap_pushReachableObject(context->heap, currentStackRecord->bytecodeMethodStackRecord.decodedArgumentsTemporaryZone[i]);

                beacon_heap_pushReachableObject(context->heap, currentStackRecord->bytecodeMethodStackRecord.returnResultValue);

            }
                break;
            
            default:
                abort();
                break;
            }
            currentStackRecord = currentStackRecord->previousRecord;
        }
    }
}

void beacon_garbageCollect_markPhase(beacon_context_t *context)
{
    beacon_garbageCollect_markRootsPhase(context);

    beacon_MemoryHeap_t *heap = context->heap;

    while(context->heap->markingStackSize > 0)
    {
        beacon_oop_t oopToExpand = context->heap->markingStack[--context->heap->markingStackSize];
        beacon_oop_t class = (beacon_oop_t)beacon_getClass(context, oopToExpand);
        beacon_heap_pushReachableObject(heap, class);

        if(beacon_isImmediate(oopToExpand))
            continue;

        beacon_ObjectHeader_t *objectToExpand = (beacon_ObjectHeader_t *)oopToExpand;
        assert(objectToExpand->gcColor != heap->whiteGCColor);
        if(objectToExpand->gcColor == heap->blackGCColor)
            continue;

        if(objectToExpand->objectKind == BeaconObjectKindPointers)
        {
            beacon_oop_t *pointers = (beacon_oop_t *)(objectToExpand + 1);
            for(size_t i = 0; i < objectToExpand->slotCount; ++i)
                beacon_heap_pushReachableObject(heap, pointers[i]);
        }

        objectToExpand->gcColor = heap->blackGCColor;
    } 
}

void beacon_garbageCollect_clearWeakObjects(beacon_context_t *context)
{
    beacon_MemoryHeap_t *heap = context->heap;
    beacon_MemoryAllocationHeader_t *position = heap->lastAllocation;
    while(position)
    {
        beacon_ObjectHeader_t *objectHeader = (beacon_ObjectHeader_t*)(position + 1);
        if(objectHeader->objectKind == BeaconObjectKindWeakPointers)
        {
            size_t slotCount = objectHeader->slotCount;
            beacon_oop_t *slots = (beacon_oop_t*)(objectHeader + 1);
            for(size_t i = 0; i < slotCount; ++i)
            {
                beacon_oop_t *slot = slots + i;
                if(beacon_isImmediate(*slot))
                    continue;;
                
                beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t *)*slot;
                if(header->gcColor != heap->blackGCColor)
                    *slot = context->roots.weakTombstone;
            }
        }
        position = position->nextAllocation;
    }
}

void beacon_garbageCollect_sweepPhase(beacon_MemoryHeap_t *heap)
{
    beacon_MemoryAllocationHeader_t *previous = heap->lastAllocation;
    beacon_MemoryAllocationHeader_t *position = heap->lastAllocation;
    while(position)
    {
        beacon_MemoryAllocationHeader_t *nextPosition = position->nextAllocation;
        beacon_ObjectHeader_t *positionHeader = (beacon_ObjectHeader_t*)position + 1;

        if(positionHeader->gcColor == heap->whiteGCColor)
        {
            size_t allocationSize = position->allocationSize;
            assert(heap->allocatedByteCount >= allocationSize);
            heap->allocatedByteCount -= allocationSize;
            if(previous == position)
            {
                free(position);
                heap->lastAllocation = nextPosition;
                previous = position = nextPosition;
            }
            else
            {
                free(position);
                if(previous)
                    previous->nextAllocation = nextPosition;
                position = nextPosition;
            }
        }
        else
        {
            previous = position;
            position = nextPosition;
        }
    }
}

void beacon_garbageCollect_swapColors(beacon_MemoryHeap_t *heap)
{
    uint8_t oldBlack = heap->blackGCColor;
    heap->blackGCColor = heap->whiteGCColor;
    heap->whiteGCColor = oldBlack;
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

    free(heap->markingStack);
    free(heap);
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

void beacon_memoryHeapSafepoint(beacon_context_t *context)
{
    beacon_MemoryHeap_t *heap = context->heap;
    if(heap->gcDisableCount > 0)
        return;

    // Check the GC activation policy.
    //size_t beforeGC = heap->allocatedByteCount;
    //size_t oldTrigger = heap->gcTriggerLimit;
    if(heap->allocatedByteCount <= heap->gcTriggerLimit)
        return;

    beacon_garbageCollect_markPhase(context);
    beacon_garbageCollect_clearWeakObjects(context);
    beacon_garbageCollect_sweepPhase(context->heap);
    beacon_garbageCollect_swapColors(context->heap);

    size_t afterGC = heap->allocatedByteCount;
    if(afterGC > heap->gcTriggerLimit)
        heap->gcTriggerLimit = afterGC*2;

    //printf("Garbage Collection before %zu - %zu after. Old trigger %zu NewTrigger %zu.\n", beforeGC, afterGC, oldTrigger, heap->gcTriggerLimit);
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
    allocation->allocationSize = allocationSize;
    heap->lastAllocation = allocation;
    heap->allocatedByteCount += allocationSize;

    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t *)(allocation + 1);
    header->behavior = behavior;
    header->objectKind = kind;
    header->gcColor = heap->whiteGCColor;
    header->slotCount = size - sizeof(beacon_ObjectHeader_t);
    if(kind != BeaconObjectKindBytes)
        header->slotCount /= sizeof(beacon_oop_t);

    return header;
}