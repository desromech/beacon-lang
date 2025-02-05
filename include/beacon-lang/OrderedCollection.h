#ifndef BEACON_ORDERED_COLLECTION_H
#define BEACON_ORDERED_COLLECTION_H

#pragma once

typedef struct beacon_context_s beacon_context_t;

#include "ObjectModel.h"
#include <stddef.h>

beacon_OrderedCollection_t *beacon_OrderedCollection_new(beacon_context_t *context);
void beacon_OrderedCollection_add(beacon_context_t *context, beacon_OrderedCollection_t *collection, beacon_oop_t element);
void beacon_OrderedCollection_size(beacon_OrderedCollection_t *collection);
beacon_oop_t beacon_OrderedCollection_at(beacon_OrderedCollection_t *collection, size_t index);

#endif //BEACON_ORDERED_COLLECTION_H