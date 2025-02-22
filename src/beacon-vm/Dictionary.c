#include "beacon-lang/Dictionary.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Exceptions.h"
#include <stdlib.h>
#include <stdio.h>

beacon_MethodDictionary_t *beacon_MethodDictionary_new(beacon_context_t *context)
{
    beacon_MethodDictionary_t *dictionary = beacon_allocateObjectWithBehavior(context->heap, context->classes.methodDictionaryClass, sizeof(beacon_MethodDictionary_t), BeaconObjectKindPointers);
    dictionary->super.super.array = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t)*16*2, BeaconObjectKindPointers);
    dictionary->super.super.tally = beacon_encodeSmallInteger(0);
    return dictionary;
}

int beacon_MethodDictionary_scanFor(beacon_MethodDictionary_t * dictionary, beacon_oop_t key)
{
    uint32_t identityHash = beacon_computeIdentityHash(key);
    beacon_Array_t *storage = dictionary->super.super.array;
    uint32_t capacity = storage->super.super.super.super.super.header.slotCount / 2;
    uint32_t keySlot = identityHash % capacity;

    for(uint32_t i = keySlot; i < capacity; ++i)
    {
        beacon_oop_t storageElement = storage->elements[i*2];
        if(!storageElement || storageElement == key)
            return i;
    }

    for(uint32_t i = 0; i < keySlot; ++i)
    {
        beacon_oop_t storageElement = storage->elements[i*2];
        if(!storageElement || storageElement == key)
            return i;
    }
    return -1;
}

void beacon_MethodDictionary_incrementCapacity(beacon_context_t *context, beacon_MethodDictionary_t *dictionary)
{
    beacon_Array_t *oldStorage = dictionary->super.super.array;
    size_t oldCapacity = 0;
    if(oldStorage)
        oldCapacity = oldStorage->super.super.super.super.super.header.slotCount / 2;

    size_t newCapacity = oldCapacity*2;
    if(newCapacity < 16)
        newCapacity = 16;

    beacon_Array_t *newStorage = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t)*newCapacity*2, BeaconObjectKindPointers);
    dictionary->super.super.array = newStorage;
    dictionary->super.super.tally = beacon_encodeSmallInteger(0);

    for(size_t i = 0; i < oldCapacity; ++i)
    {
        beacon_Symbol_t *key = (beacon_Symbol_t *)oldStorage->elements[i*2];
        beacon_oop_t value = oldStorage->elements[i*2 + 1];
        if(key)
            beacon_MethodDictionary_atPut(context, dictionary, key, value);
    }
}

void beacon_MethodDictionary_atPut(beacon_context_t *context, beacon_MethodDictionary_t *dictionary, beacon_Symbol_t *symbol, beacon_oop_t element)
{
    int slotIndex = beacon_MethodDictionary_scanFor(dictionary, (beacon_oop_t)symbol);
    BeaconAssert(context, slotIndex >= 0);

    beacon_Array_t *storage = dictionary->super.super.array;
    if(!storage->elements[slotIndex*2])
    {
        storage->elements[slotIndex*2] = (beacon_oop_t)symbol;
        storage->elements[slotIndex*2 + 1] = element;
        intptr_t newDictionarySize = beacon_decodeSmallInteger(dictionary->super.super.tally) + 1;
        dictionary->super.super.tally = beacon_encodeSmallInteger(newDictionarySize);

        size_t capacity = storage->super.super.super.super.super.header.slotCount / 2;
        size_t targetCapacity = capacity *80/100;
        if(newDictionarySize > (intptr_t)targetCapacity)
            beacon_MethodDictionary_incrementCapacity(context, dictionary);


    }
    else
    {
        storage->elements[slotIndex*2 + 1] = element;
    }

}

beacon_oop_t beacon_MethodDictionary_atOrNil(beacon_context_t *context, beacon_MethodDictionary_t *dictionary, beacon_Symbol_t *symbol)
{
    (void)context;
    int slotIndex = beacon_MethodDictionary_scanFor(dictionary, (beacon_oop_t)symbol);
    if(slotIndex < 0)
        return 0;

    return dictionary->super.super.array->elements[slotIndex*2 + 1];
}

bool beacon_MethodDictionary_includesKey(beacon_context_t *context, beacon_MethodDictionary_t *dictionary, beacon_Symbol_t *symbol)
{
    (void)context;
    int slotIndex = beacon_MethodDictionary_scanFor(dictionary, (beacon_oop_t)symbol);
    if(slotIndex < 0)
        return 0;

    return dictionary->super.super.array->elements[slotIndex*2] != 0;
}

static beacon_oop_t beacon_MethodDictionaryPrimitive_atOrNil(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 1);
    beacon_MethodDictionary_t *methodDict = (beacon_MethodDictionary_t*)receiver;
    beacon_Symbol_t *symbol = (beacon_Symbol_t *)arguments[0];
    return beacon_MethodDictionary_atOrNil(context, methodDict, symbol);
}

static beacon_oop_t beacon_MethodDictionaryPrimitive_atPut(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 2);
    beacon_MethodDictionary_t *methodDict = (beacon_MethodDictionary_t*)receiver;
    beacon_Symbol_t *symbol = (beacon_Symbol_t *)arguments[0];
    beacon_oop_t value = arguments[1];
    beacon_MethodDictionary_atPut(context, methodDict, symbol, value);
    return value;
}

void beacon_context_registerDictionaryPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.methodDictionaryClass, "atOrNil:", 1, beacon_MethodDictionaryPrimitive_atPut);
    beacon_addPrimitiveToClass(context, context->classes.methodDictionaryClass, "at:put:", 2, beacon_MethodDictionaryPrimitive_atPut);
}
