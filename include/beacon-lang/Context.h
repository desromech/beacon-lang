#ifndef BEACON_CONTEXT_H
#define BEACON_CONTEXT_H

#pragma once

#include "ObjectModel.h"
#include "Memory.h"

typedef struct beacon_context_s beacon_context_t;

struct beacon_context_s
{
    struct ContextClasses
    {
        beacon_Behavior_t *protoObjectClass;
        beacon_Behavior_t *objectClass;
        beacon_Behavior_t *collectionClass;
        beacon_Behavior_t *hashedCollectionClass;
        beacon_Behavior_t *dictionaryClass;
        beacon_Behavior_t *methodDictionaryClass;
        beacon_Behavior_t *behaviorClass;
        beacon_Behavior_t *classDescriptionClass;
        beacon_Behavior_t *classClass;
        beacon_Behavior_t *metaclassClass;

        beacon_Behavior_t *sequenceableCollectionClass;
        beacon_Behavior_t *orderedCollectionClass;
        beacon_Behavior_t *arrayedCollectionClass;
        beacon_Behavior_t *arrayClass;
        beacon_Behavior_t *byteArrayClass;
        beacon_Behavior_t *stringClass;
        beacon_Behavior_t *symbolClass;

        beacon_Behavior_t *sourceCodeClass;
        beacon_Behavior_t *sourcePositionClass;
        beacon_Behavior_t *scannerTokenClass;

        beacon_Behavior_t *parseTreeNodeClass;
        beacon_Behavior_t *parseTreeErrorNodeClass;
        beacon_Behavior_t *parseTreeLiteralNodeClass;
        beacon_Behavior_t *parseTreeIdentifierReferenceNodeClass;
        beacon_Behavior_t *parseTreeMessageSendNodeClass;
        beacon_Behavior_t *parseTreeMessageCascadeNodeClass;
        beacon_Behavior_t *parseTreeCascadedMessageNodeClass;
        beacon_Behavior_t *parseTreeSequenceNodeClass;
        beacon_Behavior_t *parseTreeReturnNodeClass;
        beacon_Behavior_t *parseTreeArrayNodeClass;
        beacon_Behavior_t *parseTreeLiteralArrayNodeClass;
        beacon_Behavior_t *parseTreeArgumentDefinitionNodeClass;
        beacon_Behavior_t *parseTreeLocalVariableDefinitionNodeClass;
        beacon_Behavior_t *parseTreeBlockClosureNodeClass;

    } classes;

    struct ContextGCRoots
    {
        beacon_InternedSymbolSet_t *internedSymbolSet;
    } roots;

    beacon_MemoryHeap_t *heap;
};

beacon_context_t *beacon_context_new(void);
void beacon_context_destroy(beacon_context_t *context);

uint32_t beacon_computeStringHash(size_t stringSize, const char *string);

beacon_String_t *beacon_importCString(beacon_context_t *context, const char *string);
beacon_Symbol_t *beacon_internStringWithSize(beacon_context_t *context, size_t stringSize, const char *string);
beacon_Symbol_t *beacon_internCString(beacon_context_t *context, const char *string);
beacon_Symbol_t *beacon_internString(beacon_context_t *context, beacon_String_t *string);


#endif // BEACON_CONTEXT_H