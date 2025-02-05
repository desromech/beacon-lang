
#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Exceptions.h"
#include "beacon-lang/ArrayList.h"

beacon_ArrayList_t *beacon_ArrayList_new(beacon_context_t *context)
{
    beacon_ArrayList_t *collection = beacon_allocateObjectWithBehavior(context->heap, context->roots.orderedCollectionClass, sizeof(beacon_ArrayList_t), BeaconObjectKindPointers);
    collection->array = beacon_allocateObjectWithBehavior(context->heap, context->roots.arrayClass, sizeof(beacon_Array_t) + 10*sizeof(beacon_oop_t), BeaconObjectKindPointers);
    collection->size = beacon_encodeSmallInteger(0);
    collection->capacity = beacon_encodeSmallInteger(10);
    return collection;
}

void beacon_ArrayList_increaseCapacity(beacon_context_t *context, beacon_ArrayList_t *collection)
{
    intptr_t oldCapacity = beacon_decodeSmallInteger(collection->capacity);
    intptr_t newCapacity = oldCapacity * 2;
    if(newCapacity < 10)
        newCapacity = 10;
    
    beacon_Array_t *oldStorage = collection->array;
    beacon_Array_t *newStorage = collection->array = beacon_allocateObjectWithBehavior(context->heap, context->roots.arrayClass, sizeof(beacon_Array_t) + newCapacity*sizeof(beacon_oop_t), BeaconObjectKindPointers);
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    for(intptr_t i = 0; i < size; ++i)
        newStorage->elements[i] = oldStorage->elements[i];

    collection->array = newStorage;
    collection->capacity = beacon_encodeSmallInteger(newCapacity);
}

void beacon_ArrayList_add(beacon_context_t *context, beacon_ArrayList_t *collection, beacon_oop_t element)
{
    if(collection->size == collection->capacity)
        beacon_ArrayList_increaseCapacity(context, collection);
    
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    collection->array->elements[size] = element;
    collection->size = beacon_encodeSmallInteger(size + 1);
}

intptr_t beacon_ArrayList_size(beacon_context_t *context, beacon_ArrayList_t *collection)
{
    (void)context;
    return beacon_decodeSmallInteger(collection->size);
}

beacon_oop_t beacon_ArrayList_at(beacon_context_t *context, beacon_ArrayList_t *collection, intptr_t index)
{
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    if(index < 1 || index >= size)
        beacon_exception_error(context, "Index out of bounds.");

    return collection->array->elements[index - 1];
}

void beacon_ArrayList_atPut(beacon_context_t *context, beacon_ArrayList_t *collection, intptr_t index, beacon_oop_t element)
{
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    if(index < 1 || index >= size)
        beacon_exception_error(context, "Index out of bounds.");
    collection->array->elements[index - 1] = element;
}