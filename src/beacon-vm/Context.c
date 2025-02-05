#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
#include <stdlib.h>
#include <string.h>

static beacon_Behavior_t *beacon_context_createClassAndMetaclass(beacon_context_t *context, beacon_Behavior_t *superclassBehavior, const char *name)
{
    beacon_Metaclass_t *metaclass = beacon_allocateObjectWithBehavior(context->heap, context->roots.metaclassClass, sizeof(beacon_Metaclass_t), BeaconObjectKindPointers);
    beacon_Class_t *clazz = beacon_allocateObjectWithBehavior(context->heap, (beacon_Behavior_t*)metaclass, sizeof(beacon_Class_t), BeaconObjectKindPointers);
    metaclass->thisClass = clazz;
    if(name)
        clazz->name = beacon_internCString(context, name);

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
    earlyMetaClass->super.super.header.behavior = context->roots.metaclassClass;
}
static void beacon_context_createBaseClassHierarchy(beacon_context_t *context)
{
    // Meta-circular hierarchy.
    context->roots.protoObjectClass = beacon_context_createClassAndMetaclass(context, NULL, "ProtoObject");
    context->roots.objectClass = beacon_context_createClassAndMetaclass(context, context->roots.protoObjectClass, "Object");
    context->roots.behaviorClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "Behavior");
    context->roots.classDescriptionClass = beacon_context_createClassAndMetaclass(context, context->roots.behaviorClass, "ClassDescription");
    context->roots.metaclassClass = beacon_context_createClassAndMetaclass(context, context->roots.classDescriptionClass, "Metaclass");
    
    beacon_context_fixEarlyMetaclass(context, context->roots.protoObjectClass);
    beacon_context_fixEarlyMetaclass(context, context->roots.objectClass);
    beacon_context_fixEarlyMetaclass(context, context->roots.behaviorClass);
    beacon_context_fixEarlyMetaclass(context, context->roots.classDescriptionClass);
    beacon_context_fixEarlyMetaclass(context, context->roots.metaclassClass);
    context->roots.classClass = beacon_context_createClassAndMetaclass(context, context->roots.classDescriptionClass, "Class");
    context->roots.protoObjectClass->super.super.header.behavior->superclass = context->roots.classClass;

    // Normal class orders
    context->roots.collectionClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "Collection");
    context->roots.hashedCollectionClass = beacon_context_createClassAndMetaclass(context, context->roots.collectionClass, "HashedCollection");
    context->roots.dictionaryClass = beacon_context_createClassAndMetaclass(context, context->roots.hashedCollectionClass, "Dictionary");
    context->roots.methodDictionaryClass = beacon_context_createClassAndMetaclass(context, context->roots.methodDictionaryClass, "MethodDictionary");
    context->roots.sequenceableCollectionClass = beacon_context_createClassAndMetaclass(context, context->roots.collectionClass, "SequenceableCollection");
    context->roots.orderedCollectionClass = beacon_context_createClassAndMetaclass(context, context->roots.sequenceableCollectionClass, "OrderedCollection");
    context->roots.arrayedCollectionClass = beacon_context_createClassAndMetaclass(context, context->roots.sequenceableCollectionClass, "ArrayedCollection");
    context->roots.arrayClass = beacon_context_createClassAndMetaclass(context, context->roots.arrayedCollectionClass, "Array");
    context->roots.stringClass = beacon_context_createClassAndMetaclass(context, context->roots.arrayedCollectionClass, "String");
    context->roots.symbolClass = beacon_context_createClassAndMetaclass(context, context->roots.arrayedCollectionClass, "Symbol");

    context->roots.sourceCodeClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "SourceCode");
    context->roots.sourcePositionClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "SourcePosition");
    context->roots.scannerTokenClass = beacon_context_createClassAndMetaclass(context, context->roots.scannerTokenClass, "ScannerToken");
}

beacon_context_t *beacon_context_new(void)
{
    beacon_context_t *context = calloc(1, sizeof(beacon_context_t));
    context->heap = beacon_createMemoryHeap(context);
    beacon_context_createBaseClassHierarchy(context);
    return context;
}

void beacon_context_destroy(beacon_context_t *context)
{
    beacon_destroyMemoryHeap(context->heap);
    free(context);
}

beacon_String_t *beacon_importCString(beacon_context_t *context, const char *string)
{
    size_t stringSize = strlen(string);
    beacon_String_t *importedString = beacon_allocateObjectWithBehavior(context->heap, context->roots.stringClass, sizeof(beacon_String_t) + stringSize, BeaconObjectKindBytes);
    memcpy(importedString->data, string, stringSize);
    return importedString;
}

beacon_Symbol_t *beacon_internCString(beacon_context_t *context, const char *string)
{
    size_t stringSize = strlen(string);
    beacon_Symbol_t *importedString = beacon_allocateObjectWithBehavior(context->heap, context->roots.symbolClass, sizeof(beacon_Symbol_t) + stringSize, BeaconObjectKindBytes);
    memcpy(importedString->data, string, stringSize);
    return importedString;
}