#ifndef BEACON_LANG_DYNARRAY_CONTEXT_H
#define BEACON_LANG_DYNARRAY_CONTEXT_H

#include <stddef.h>
#include <stdint.h>

typedef struct beacon_DynArray_s
{
    size_t entrySize;
    size_t size;
    size_t capacity;
    uint8_t *data;
} beacon_DynArray_t;

void beacon_DynArray_initialize(beacon_DynArray_t *dynarray, size_t entrySize, size_t initialCapacity);
size_t beacon_DynArray_addAll(beacon_DynArray_t *dynarray, size_t entryCount, const void *newEntries);
size_t beacon_DynArray_add(beacon_DynArray_t *dynarray, const void *newEntry);
void beacon_DynArray_destroy(beacon_DynArray_t *dynarray);

#define beacon_DynArray_entryOfTypeAt(dynarray, entryType, index) (((entryType*)(dynarray).data) + index)

#endif //beacon_DYNARRAY_CONTEXT_H