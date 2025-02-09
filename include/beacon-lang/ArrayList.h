#ifndef BEACON_ARRAY_LIST_H
#define BEACON_ARRAY_LIST_H

#pragma once

#include "ObjectModel.h"
#include <stddef.h>

typedef struct beacon_context_s beacon_context_t;

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

#endif //BEACON_ARRAY_LIST_H