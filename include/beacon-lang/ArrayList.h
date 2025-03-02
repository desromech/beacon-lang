#ifndef BEACON_ARRAY_LIST_H
#define BEACON_ARRAY_LIST_H

#pragma once

#include "ObjectModel.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct beacon_context_s beacon_context_t;

/**
 * Make an array with the specified element.
 */
beacon_Array_t *beacon_Array_copyWith(beacon_context_t *context, beacon_Array_t *originalArray, beacon_oop_t value);

/**
 * Constructs a new array list.
 */
beacon_ArrayList_t *beacon_ArrayList_new(beacon_context_t *context);

/**
 * Adds a single element to the array list.
 */
void beacon_ArrayList_add(beacon_context_t *context, beacon_ArrayList_t *collection, beacon_oop_t element);

/**
 * Gets the size of the array list.
 */
beacon_Array_t *beacon_ArrayList_asArray(beacon_context_t *context, beacon_ArrayList_t *collection);

/**
 * Gets the size of the array list.
 */
intptr_t beacon_ArrayList_size(beacon_ArrayList_t *collection);

/**
 * Gets the element at the specified location.
 */
beacon_oop_t beacon_ArrayList_at(beacon_context_t *context, beacon_ArrayList_t *collection, intptr_t index);

/**
 * Sets the element at the specified location.
 */
void beacon_ArrayList_atPut(beacon_context_t *context, beacon_ArrayList_t *collection, intptr_t index, beacon_oop_t element);

/**
 * Constructs a new array list.
 */
beacon_ByteArrayList_t *beacon_ByteArrayList_new(beacon_context_t *context);

/**
 * Adds a single element to the array list.
 */
void beacon_ByteArrayList_add(beacon_context_t *context, beacon_ByteArrayList_t *collection, uint8_t element);

/**
 * Adds a single element to the array list.
 */
void beacon_ByteArrayList_addInt16(beacon_context_t *context, beacon_ByteArrayList_t *collection, int16_t element);

/**
 * Adds a single element to the array list.
 */
void beacon_ByteArrayList_addUInt16(beacon_context_t *context, beacon_ByteArrayList_t *collection, uint16_t element);


/**
 * Gets the size of the array list.
 */
beacon_ByteArray_t *beacon_ByteArrayList_asByteArray(beacon_context_t *context, beacon_ByteArrayList_t *collection);

/**
 * Gets the size of the array list.
 */
intptr_t beacon_ByteArrayList_size(beacon_ByteArrayList_t *collection);

/**
 * Gets the element at the specified location.
 */
uint8_t beacon_ByteArrayList_at(beacon_context_t *context, beacon_ByteArrayList_t *collection, intptr_t index);

/**
 * Sets the element at the specified location.
 */
void beacon_ByteArrayList_atPut(beacon_context_t *context, beacon_ByteArrayList_t *collection, intptr_t index, uint8_t element);

#ifdef __cplusplus
}
#endif

#endif //BEACON_ARRAY_LIST_H