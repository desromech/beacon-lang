#ifndef BEACON_LANG_DICTIONARY_H
#define BEACON_LANG_DICTIONARY_H

#include "ObjectModel.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct beacon_context_s beacon_context_t;


/**
 * Instantiate a new method dictionary.
 */
beacon_MethodDictionary_t *beacon_MethodDictionary_new(beacon_context_t *context);

/**
 * Add an element to the dictionary.
 */
void beacon_MethodDictionary_atPut(beacon_context_t *context, beacon_MethodDictionary_t *dictionary, beacon_Symbol_t *symbol, beacon_oop_t element);

/**
 * Looks for a given value, or return nil.
 */
beacon_oop_t beacon_MethodDictionary_atOrNil(beacon_context_t *context, beacon_MethodDictionary_t *dictionary, beacon_Symbol_t *symbol);

/**
 * Checks for the inclusion of a key
 */
bool beacon_MethodDictionary_includesKey(beacon_context_t *context, beacon_MethodDictionary_t *dictionary, beacon_Symbol_t *symbol);

#ifdef __cplusplus
}
#endif

#endif //BEACON_LANG_DICTIONARY_H