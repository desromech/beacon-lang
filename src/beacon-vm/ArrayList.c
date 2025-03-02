
#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Exceptions.h"
#include "beacon-lang/ArrayList.h"

beacon_Array_t *beacon_Array_copyWith(beacon_context_t *context, beacon_Array_t *originalArray, beacon_oop_t value)
{
    if(!originalArray || originalArray->super.super.super.super.super.header.slotCount == 0)
    {
        beacon_Array_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t), BeaconObjectKindPointers);
        result->elements[0] = value;
        return result;
    }

    size_t originalArraySize = originalArray->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < originalArraySize; ++i)
    {
        if(originalArray->elements[i] == value)
            return originalArray;
    }

    beacon_Array_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + (originalArraySize +1 )*sizeof(beacon_oop_t), BeaconObjectKindPointers);
    for(size_t i = 0; i < originalArraySize; ++i)
        result->elements[i] = originalArray->elements[i];
    result->elements[originalArraySize] = value;
    return result;
}

beacon_ArrayList_t *beacon_ArrayList_new(beacon_context_t *context)
{
    beacon_ArrayList_t *collection = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayListClass, sizeof(beacon_ArrayList_t), BeaconObjectKindPointers);
    collection->array = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + 10*sizeof(beacon_oop_t), BeaconObjectKindPointers);
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
    beacon_Array_t *newStorage = collection->array = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + newCapacity*sizeof(beacon_oop_t), BeaconObjectKindPointers);
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

void beacon_ArrayList_addAfter(beacon_context_t *context, beacon_ArrayList_t *collection, beacon_oop_t element, size_t insertionIndex)
{
    if(collection->size == collection->capacity)
        beacon_ArrayList_increaseCapacity(context, collection);
    
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    intptr_t moveElementCount = size - insertionIndex;
    for(intptr_t i = 0; i < moveElementCount; ++i)
        collection->array->elements[size - i] = collection->array->elements[size - i - 1];

    collection->array->elements[insertionIndex] = element;
    collection->size = beacon_encodeSmallInteger(size + 1);
}

void beacon_ArrayList_removeAt(beacon_context_t *context, beacon_ArrayList_t *collection, size_t removeIndex)
{
    if(collection->size == collection->capacity)
        beacon_ArrayList_increaseCapacity(context, collection);
    
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    for(intptr_t i = removeIndex - 1; i < size; ++i)
        collection->array->elements[i] = collection->array->elements[i + 1];

    collection->size = beacon_encodeSmallInteger(size - 1);
}

beacon_Array_t *beacon_ArrayList_asArray(beacon_context_t *context, beacon_ArrayList_t *collection)
{
    intptr_t size = beacon_ArrayList_size(collection);
    beacon_Array_t *array = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t)*size, BeaconObjectKindPointers);
    for(intptr_t i = 0; i < size; ++i)
        array->elements[i] = collection->array->elements[i];
    return array;
}

intptr_t beacon_ArrayList_size(beacon_ArrayList_t *collection)
{
    return beacon_decodeSmallInteger(collection->size);
}

beacon_oop_t beacon_ArrayList_at(beacon_context_t *context, beacon_ArrayList_t *collection, intptr_t index)
{
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    if(index < 1 || index > size)
        beacon_exception_error(context, "Index out of bounds.");

    return collection->array->elements[index - 1];
}

void beacon_ArrayList_atPut(beacon_context_t *context, beacon_ArrayList_t *collection, intptr_t index, beacon_oop_t element)
{
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    if(index < 1 || index > size)
        beacon_exception_error(context, "Index out of bounds.");
    collection->array->elements[index - 1] = element;
}

// ByteArrayList
beacon_ByteArrayList_t *beacon_ByteArrayList_new(beacon_context_t *context)
{
    beacon_ByteArrayList_t *collection = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayListClass, sizeof(beacon_ByteArrayList_t), BeaconObjectKindPointers);
    collection->array = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, sizeof(beacon_ByteArray_t) + 32, BeaconObjectKindBytes);
    collection->size = beacon_encodeSmallInteger(0);
    collection->capacity = beacon_encodeSmallInteger(32);
    return collection;
}

void beacon_ByteArrayList_increaseCapacity(beacon_context_t *context, beacon_ByteArrayList_t *collection)
{
    intptr_t oldCapacity = beacon_decodeSmallInteger(collection->capacity);
    intptr_t newCapacity = oldCapacity * 2;
    if(newCapacity < 32)
        newCapacity = 32;
    
    beacon_ByteArray_t *oldStorage = collection->array;
    beacon_ByteArray_t *newStorage = collection->array = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, sizeof(beacon_ByteArray_t) + newCapacity, BeaconObjectKindBytes);
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    for(intptr_t i = 0; i < size; ++i)
        newStorage->elements[i] = oldStorage->elements[i];

    collection->array = newStorage;
    collection->capacity = beacon_encodeSmallInteger(newCapacity);
}

void beacon_ByteArrayList_add(beacon_context_t *context, beacon_ByteArrayList_t *collection, uint8_t element)
{
    if(collection->size == collection->capacity)
        beacon_ByteArrayList_increaseCapacity(context, collection);
    
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    collection->array->elements[size] = element;
    collection->size = beacon_encodeSmallInteger(size + 1);
}

void beacon_ByteArrayList_addInt16(beacon_context_t *context, beacon_ByteArrayList_t *collection, int16_t element)
{
    beacon_ByteArrayList_add(context, collection, element & 0xFF);
    beacon_ByteArrayList_add(context, collection, (element >> 8) & 0xFF);
}

void beacon_ByteArrayList_addUInt16(beacon_context_t *context, beacon_ByteArrayList_t *collection, uint16_t element)
{
    beacon_ByteArrayList_add(context, collection, element & 0xFF);
    beacon_ByteArrayList_add(context, collection, (element >> 8) & 0xFF);
}

beacon_ByteArray_t *beacon_ByteArrayList_asByteArray(beacon_context_t *context, beacon_ByteArrayList_t *collection)
{
    intptr_t size = beacon_ByteArrayList_size(collection);
    beacon_ByteArray_t *byteArray = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, sizeof(beacon_ByteArray_t) + size, BeaconObjectKindBytes);
    memcpy(byteArray->elements, collection->array->elements, size);
    return byteArray;
}

intptr_t beacon_ByteArrayList_size(beacon_ByteArrayList_t *collection)
{
    return beacon_decodeSmallInteger(collection->size);
}

uint8_t beacon_ByteArrayList_at(beacon_context_t *context, beacon_ByteArrayList_t *collection, intptr_t index)
{
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    if(index < 1 || index > size)
        beacon_exception_error(context, "Index out of bounds.");

    return collection->array->elements[index - 1];
}

void beacon_ByteArrayList_atPut(beacon_context_t *context, beacon_ByteArrayList_t *collection, intptr_t index, uint8_t element)
{
    intptr_t size = beacon_decodeSmallInteger(collection->size);
    if(index < 1 || index > size)
        beacon_exception_error(context, "Index out of bounds.");
    collection->array->elements[index - 1] = element;
}

static beacon_oop_t beacon_ArrayList_addPrimitive(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 1);
    beacon_ArrayList_t *arrayList = (beacon_ArrayList_t*)receiver;
    beacon_ArrayList_add(context, arrayList, arguments[0]);
    return arguments[0];
}

static beacon_oop_t beacon_ArrayList_addAfterPrimitive(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 2);
    beacon_ArrayList_t *arrayList = (beacon_ArrayList_t*)receiver;
    beacon_ArrayList_addAfter(context, arrayList, arguments[0], beacon_decodeSmallInteger(arguments[1]));
    return arguments[0];
}

static beacon_oop_t beacon_ArrayList_removeAtPrimitive(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 1);
    beacon_ArrayList_t *arrayList = (beacon_ArrayList_t*)receiver;
    beacon_ArrayList_removeAt(context, arrayList, beacon_decodeSmallInteger(arguments[0]));
    return receiver;
}

static beacon_oop_t beacon_ArrayList_asArrayPrimitive(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 0);
    beacon_ArrayList_t *arrayList = (beacon_ArrayList_t*)receiver;
    return (beacon_oop_t)beacon_ArrayList_asArray(context, arrayList);
}

void beacon_context_registerArrayListPrimitive(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.arrayListClass, "add:", 1, beacon_ArrayList_addPrimitive);
    beacon_addPrimitiveToClass(context, context->classes.arrayListClass, "add:after:", 2, beacon_ArrayList_addAfterPrimitive);
    beacon_addPrimitiveToClass(context, context->classes.arrayListClass, "removeAt:", 2, beacon_ArrayList_removeAtPrimitive);
    beacon_addPrimitiveToClass(context, context->classes.arrayListClass, "asArray", 0, beacon_ArrayList_asArrayPrimitive);
}
