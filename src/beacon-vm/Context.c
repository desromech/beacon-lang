#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static beacon_Behavior_t *beacon_context_createClassAndMetaclass(beacon_context_t *context, beacon_Behavior_t *superclassBehavior, const char *name, size_t instanceSize, beacon_ObjectKind_t objectKind)
{
    assert(instanceSize >= sizeof(beacon_ObjectHeader_t));

    beacon_Metaclass_t *metaclass = beacon_allocateObjectWithBehavior(context->heap, context->classes.metaclassClass, sizeof(beacon_Metaclass_t), BeaconObjectKindPointers);
    metaclass->super.super.instSize = beacon_encodeSmallInteger(sizeof(beacon_Class_t) - sizeof(beacon_ObjectHeader_t));
    metaclass->super.super.objectKind = beacon_encodeSmallInteger(BeaconObjectKindPointers);

    beacon_Class_t *clazz = beacon_allocateObjectWithBehavior(context->heap, (beacon_Behavior_t*)metaclass, sizeof(beacon_Class_t), BeaconObjectKindPointers);
    metaclass->thisClass = clazz;
    if(name)
        clazz->name = beacon_internCString(context, name);
    clazz->super.super.instSize = beacon_encodeSmallInteger(instanceSize - sizeof(beacon_ObjectHeader_t));
    clazz->super.super.objectKind = beacon_encodeSmallInteger(objectKind);

    if(superclassBehavior)
    {
        clazz->super.super.superclass = superclassBehavior;
        metaclass->super.super.superclass = superclassBehavior->super.super.header.behavior;
    }

    return &clazz->super.super;
}

static void beacon_context_fixEarlyMetaclass(beacon_context_t *context, beacon_Behavior_t *earlyClassBehavior)
{
    beacon_Behavior_t *earlyMetaClass = earlyClassBehavior->super.super.header.behavior;
    earlyMetaClass->super.super.header.behavior = context->classes.metaclassClass;
}
static void beacon_context_createBaseClassHierarchy(beacon_context_t *context)
{
    // Meta-circular hierarchy.
    context->classes.protoObjectClass = beacon_context_createClassAndMetaclass(context, NULL, "ProtoObject", sizeof(beacon_ProtoObject_t), BeaconObjectKindPointers);
    context->classes.objectClass = beacon_context_createClassAndMetaclass(context, context->classes.protoObjectClass, "Object", sizeof(beacon_Object_t), BeaconObjectKindPointers);
    context->classes.behaviorClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Behavior", sizeof(beacon_Behavior_t), BeaconObjectKindPointers);
    context->classes.classDescriptionClass = beacon_context_createClassAndMetaclass(context, context->classes.behaviorClass, "ClassDescription", sizeof(beacon_ClassDescription_t), BeaconObjectKindPointers);
    context->classes.metaclassClass = beacon_context_createClassAndMetaclass(context, context->classes.classDescriptionClass, "Metaclass", sizeof(beacon_Metaclass_t), BeaconObjectKindPointers);
    
    beacon_context_fixEarlyMetaclass(context, context->classes.protoObjectClass);
    beacon_context_fixEarlyMetaclass(context, context->classes.objectClass);
    beacon_context_fixEarlyMetaclass(context, context->classes.behaviorClass);
    beacon_context_fixEarlyMetaclass(context, context->classes.classDescriptionClass);
    beacon_context_fixEarlyMetaclass(context, context->classes.metaclassClass);
    context->classes.classClass = beacon_context_createClassAndMetaclass(context, context->classes.classDescriptionClass, "Class", sizeof(beacon_Class_t), BeaconObjectKindPointers);
    context->classes.protoObjectClass->super.super.header.behavior->superclass = context->classes.classClass;

    // Normal class orders
    context->classes.collectionClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Collection", sizeof(beacon_Collection_t), BeaconObjectKindPointers);
    context->classes.hashedCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.collectionClass, "HashedCollection", sizeof(beacon_HashedCollection_t), BeaconObjectKindPointers);
    context->classes.dictionaryClass = beacon_context_createClassAndMetaclass(context, context->classes.hashedCollectionClass, "Dictionary", sizeof(beacon_Dictionary_t), BeaconObjectKindPointers);
    context->classes.methodDictionaryClass = beacon_context_createClassAndMetaclass(context, context->classes.methodDictionaryClass, "MethodDictionary", sizeof(beacon_MethodDictionary_t), BeaconObjectKindPointers);
    context->classes.sequenceableCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.collectionClass, "SequenceableCollection", sizeof(beacon_SequenceableCollection_t), BeaconObjectKindPointers);
    context->classes.orderedCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.sequenceableCollectionClass, "ArrayList", sizeof(beacon_ArrayList_t), BeaconObjectKindPointers);
    context->classes.arrayedCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.sequenceableCollectionClass, "ArrayedCollection", sizeof(beacon_ArrayedCollection_t), BeaconObjectKindPointers);
    context->classes.arrayClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "Array", sizeof(beacon_Array_t), BeaconObjectKindPointers);
    context->classes.byteArrayClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "ByteArray", sizeof(beacon_ByteArray_t), BeaconObjectKindBytes);
    context->classes.stringClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "String", sizeof(beacon_String_t), BeaconObjectKindBytes);
    context->classes.symbolClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "Symbol", sizeof(beacon_Symbol_t), BeaconObjectKindBytes);

    context->classes.sourceCodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "SourceCode", sizeof(beacon_SourceCode_t), BeaconObjectKindPointers);
    context->classes.sourcePositionClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "SourcePosition", sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
    context->classes.scannerTokenClass = beacon_context_createClassAndMetaclass(context, context->classes.scannerTokenClass, "ScannerToken", sizeof(beacon_ScannerToken_t), BeaconObjectKindPointers);

    context->classes.parseTreeNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "ParseTreeNode", sizeof(beacon_ParseTreeNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeErrorNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeErrorNode", sizeof(beacon_ParseTreeErrorNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeLiteralNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeLiteralNode", sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeIdentifierReferenceNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeIdentifierReferenceNode", sizeof(beacon_ParseTreeIdentifierReferenceNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeMessageSendNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeMessageSendNode", sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeMessageCascadeNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeMessageCascadeNode", sizeof(beacon_ParseTreeMessageCascadeNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeCascadedMessageNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeCascadedMessageNpde", sizeof(beacon_ParseTreeCascadedMessageNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeSequenceNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeSequenceNode", sizeof(beacon_ParseTreeSequenceNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeReturnNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeReturnNode", sizeof(beacon_ParseTreeReturnNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeArrayNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeArrayNode", sizeof(beacon_ParseTreeArrayNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeLiteralArrayNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeLiteralArrayNode", sizeof(beacon_ParseTreeLiteralArrayNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeArgumentDefinitionNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeArgumentDefinitionNode", sizeof(beacon_ParseTreeArgumentDefinitionNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeLocalVariableDefinitionNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeLocalVariableDefinitionNode", sizeof(beacon_ParseTreeLocalVariableDefinitionNode_t), BeaconObjectKindPointers);
    context->classes.parseTreeBlockClosureNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeBlockClosureNode", sizeof(beacon_ParseTreeBlockClosureNode_t), BeaconObjectKindPointers);
}

beacon_context_t *beacon_context_new(void)
{
    beacon_context_t *context = calloc(1, sizeof(beacon_context_t));
    context->heap = beacon_createMemoryHeap(context);
    context->roots.internedSymbolSet = beacon_allocateObject(context->heap, sizeof(beacon_InternedSymbolSet_t), BeaconObjectKindPointers);
    context->roots.internedSymbolSet->super.array = beacon_allocateObject(context->heap, sizeof(beacon_Array_t) + sizeof(beacon_oop_t)*1024, BeaconObjectKindPointers);
    context->roots.internedSymbolSet->super.tally = beacon_encodeSmallInteger(0);
    beacon_context_createBaseClassHierarchy(context);
    return context;
}

void beacon_context_destroy(beacon_context_t *context)
{
    beacon_destroyMemoryHeap(context->heap);
    free(context);
}

uint32_t beacon_computeStringHash(size_t stringSize, const char *string)
{
    uint32_t hash = stringSize*1664525;
    for(size_t i = 0; i < stringSize; ++i)
        hash = (hash + string[i])*1664525;
    return hash;
}

beacon_String_t *beacon_importCString(beacon_context_t *context, const char *string)
{
    size_t stringSize = strlen(string);
    beacon_String_t *importedString = beacon_allocateObjectWithBehavior(context->heap, context->classes.stringClass, sizeof(beacon_String_t) + stringSize, BeaconObjectKindBytes);
    memcpy(importedString->data, string, stringSize);
    return importedString;
}

intptr_t beacon_InternedSymbolSet_scanForString(beacon_InternedSymbolSet_t *symbolSet, size_t stringSize, const char *string)
{
    beacon_Array_t *storage = symbolSet->super.array;
    size_t capacity = storage->super.super.super.super.super.header.slotCount;
    size_t stringHash = beacon_computeStringHash(stringSize, string);

    size_t naturalSlot = stringHash % capacity;
    for(size_t i = naturalSlot; i < capacity; ++i)
    {
        beacon_Symbol_t *storedSymbol = (beacon_Symbol_t *)storage->elements[i];
        if(beacon_isNil((beacon_oop_t)storedSymbol) || (storedSymbol->super.super.super.super.super.header.slotCount == stringSize
                            && !memcmp(storedSymbol, string, stringSize)))
            return i;
    }

    for(size_t i = 0; i < naturalSlot; ++i)
    {
        beacon_Symbol_t *storedSymbol = (beacon_Symbol_t *)storage->elements[i];
        if(beacon_isNil((beacon_oop_t)storedSymbol) || (storedSymbol->super.super.super.super.super.header.slotCount == stringSize
                            && !memcmp(storedSymbol, string, stringSize)))
            return i;
    }

    return -1;
}

void beacon_InternedSymbolSet_incrementCapacity(beacon_context_t *context, beacon_InternedSymbolSet_t *symbolSet)
{
    (void)context;
    (void)symbolSet;
    fprintf(stderr, "TODO: beacon_InternedSymbolSet_incrementCapacity");
}

beacon_Symbol_t *beacon_internStringWithSize(beacon_context_t *context, size_t stringSize, const char *string)
{
    intptr_t symbolSetPosition = beacon_InternedSymbolSet_scanForString(context->roots.internedSymbolSet, stringSize, string);
    if(symbolSetPosition < 0)
    {
        beacon_InternedSymbolSet_incrementCapacity(context, context->roots.internedSymbolSet);
        symbolSetPosition = beacon_InternedSymbolSet_scanForString(context->roots.internedSymbolSet, stringSize, string);
        assert(symbolSetPosition >= 0);
    }

    if(beacon_isNotNil(context->roots.internedSymbolSet->super.array->elements[symbolSetPosition]))
        return (beacon_Symbol_t *)context->roots.internedSymbolSet->super.array->elements[symbolSetPosition];


    beacon_Symbol_t *internedSymbol = beacon_allocateObjectWithBehavior(context->heap, context->classes.symbolClass, sizeof(beacon_Symbol_t) + stringSize, BeaconObjectKindBytes);
    memcpy(internedSymbol->data, string, stringSize);
    context->roots.internedSymbolSet->super.array->elements[symbolSetPosition] = (beacon_oop_t)internedSymbol;
    context->roots.internedSymbolSet->super.tally = beacon_encodeSmallInteger(
        beacon_decodeSmallInteger(context->roots.internedSymbolSet->super.tally) + 1
    );

    return internedSymbol;
}

beacon_Symbol_t *beacon_internCString(beacon_context_t *context, const char *string)
{
    return beacon_internStringWithSize(context, strlen(string), string);
}

beacon_Symbol_t *beacon_internString(beacon_context_t *context, beacon_String_t *string)
{
    size_t stringSize = string->super.super.super.super.super.header.slotCount;
    return beacon_internStringWithSize(context, stringSize, (const char *)string->data);
}