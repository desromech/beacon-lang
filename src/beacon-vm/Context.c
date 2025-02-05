#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static beacon_Behavior_t *beacon_context_createClassAndMetaclass(beacon_context_t *context, beacon_Behavior_t *superclassBehavior, const char *name, size_t instanceSize, beacon_ObjectKind_t objectKind)
{
    assert(instanceSize >= sizeof(beacon_ObjectHeader_t));

    beacon_Metaclass_t *metaclass = beacon_allocateObjectWithBehavior(context->heap, context->roots.metaclassClass, sizeof(beacon_Metaclass_t), BeaconObjectKindPointers);
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
    earlyMetaClass->super.super.header.behavior = context->roots.metaclassClass;
}
static void beacon_context_createBaseClassHierarchy(beacon_context_t *context)
{
    // Meta-circular hierarchy.
    context->roots.protoObjectClass = beacon_context_createClassAndMetaclass(context, NULL, "ProtoObject", sizeof(beacon_ProtoObject_t), BeaconObjectKindPointers);
    context->roots.objectClass = beacon_context_createClassAndMetaclass(context, context->roots.protoObjectClass, "Object", sizeof(beacon_Object_t), BeaconObjectKindPointers);
    context->roots.behaviorClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "Behavior", sizeof(beacon_Behavior_t), BeaconObjectKindPointers);
    context->roots.classDescriptionClass = beacon_context_createClassAndMetaclass(context, context->roots.behaviorClass, "ClassDescription", sizeof(beacon_ClassDescription_t), BeaconObjectKindPointers);
    context->roots.metaclassClass = beacon_context_createClassAndMetaclass(context, context->roots.classDescriptionClass, "Metaclass", sizeof(beacon_Metaclass_t), BeaconObjectKindPointers);
    
    beacon_context_fixEarlyMetaclass(context, context->roots.protoObjectClass);
    beacon_context_fixEarlyMetaclass(context, context->roots.objectClass);
    beacon_context_fixEarlyMetaclass(context, context->roots.behaviorClass);
    beacon_context_fixEarlyMetaclass(context, context->roots.classDescriptionClass);
    beacon_context_fixEarlyMetaclass(context, context->roots.metaclassClass);
    context->roots.classClass = beacon_context_createClassAndMetaclass(context, context->roots.classDescriptionClass, "Class", sizeof(beacon_Class_t), BeaconObjectKindPointers);
    context->roots.protoObjectClass->super.super.header.behavior->superclass = context->roots.classClass;

    // Normal class orders
    context->roots.collectionClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "Collection", sizeof(beacon_Collection_t), BeaconObjectKindPointers);
    context->roots.hashedCollectionClass = beacon_context_createClassAndMetaclass(context, context->roots.collectionClass, "HashedCollection", sizeof(beacon_HashedCollection_t), BeaconObjectKindPointers);
    context->roots.dictionaryClass = beacon_context_createClassAndMetaclass(context, context->roots.hashedCollectionClass, "Dictionary", sizeof(beacon_Dictionary_t), BeaconObjectKindPointers);
    context->roots.methodDictionaryClass = beacon_context_createClassAndMetaclass(context, context->roots.methodDictionaryClass, "MethodDictionary", sizeof(beacon_MethodDictionary_t), BeaconObjectKindPointers);
    context->roots.sequenceableCollectionClass = beacon_context_createClassAndMetaclass(context, context->roots.collectionClass, "SequenceableCollection", sizeof(beacon_SequenceableCollection_t), BeaconObjectKindPointers);
    context->roots.orderedCollectionClass = beacon_context_createClassAndMetaclass(context, context->roots.sequenceableCollectionClass, "ArrayList", sizeof(beacon_ArrayList_t), BeaconObjectKindPointers);
    context->roots.arrayedCollectionClass = beacon_context_createClassAndMetaclass(context, context->roots.sequenceableCollectionClass, "ArrayedCollection", sizeof(beacon_ArrayedCollection_t), BeaconObjectKindPointers);
    context->roots.arrayClass = beacon_context_createClassAndMetaclass(context, context->roots.arrayedCollectionClass, "Array", sizeof(beacon_Array_t), BeaconObjectKindPointers);
    context->roots.byteArrayClass = beacon_context_createClassAndMetaclass(context, context->roots.arrayedCollectionClass, "ByteArray", sizeof(beacon_ByteArray_t), BeaconObjectKindBytes);
    context->roots.stringClass = beacon_context_createClassAndMetaclass(context, context->roots.arrayedCollectionClass, "String", sizeof(beacon_String_t), BeaconObjectKindBytes);
    context->roots.symbolClass = beacon_context_createClassAndMetaclass(context, context->roots.arrayedCollectionClass, "Symbol", sizeof(beacon_Symbol_t), BeaconObjectKindBytes);

    context->roots.sourceCodeClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "SourceCode", sizeof(beacon_SourceCode_t), BeaconObjectKindPointers);
    context->roots.sourcePositionClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "SourcePosition", sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
    context->roots.scannerTokenClass = beacon_context_createClassAndMetaclass(context, context->roots.scannerTokenClass, "ScannerToken", sizeof(beacon_ScannerToken_t), BeaconObjectKindPointers);

    context->roots.parseTreeNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.objectClass, "ParseTreeNode", sizeof(beacon_ParseTreeNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeLiteralNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeLiteralNode", sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeIdentifierReferenceNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeIdentifierReferenceNode", sizeof(beacon_ParseTreeIdentifierReferenceNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeMessageSendNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeMessageSendNode", sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeMessageCascadeNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeMessageCascadeNode", sizeof(beacon_ParseTreeMessageCascadeNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeCascadedMessageNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeCascadedMessageNpde", sizeof(beacon_ParseTreeCascadedMessageNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeSequenceNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeSequenceNode", sizeof(beacon_ParseTreeSequenceNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeReturnNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeReturnNode", sizeof(beacon_ParseTreeReturnNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeArrayNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeArrayNode", sizeof(beacon_ParseTreeArrayNode_t), BeaconObjectKindPointers);
    context->roots.parseTreeLiteralArrayNodeClass = beacon_context_createClassAndMetaclass(context, context->roots.parseTreeNodeClass, "ParseTreeLiteralArrayNode", sizeof(beacon_ParseTreeLiteralArrayNode_t), BeaconObjectKindPointers);
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