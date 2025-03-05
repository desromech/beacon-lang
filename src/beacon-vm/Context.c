#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Dictionary.h"
#include "beacon-lang/Exceptions.h"
#include "beacon-lang/Bytecode.h"
#include "beacon-lang/ArrayList.h"
#include "beacon-lang/SourceCode.h"
#include "beacon-lang/AgpuRendering.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

void beacon_context_registerObjectBasicPrimitives(beacon_context_t *context);
void beacon_context_registerArrayListPrimitive(beacon_context_t *context);
void beacon_context_registerDictionaryPrimitives(beacon_context_t *context);
void beacon_context_registerExceptionPrimitives(beacon_context_t *context);
void beacon_context_registerAgpuRenderingPrimitives(beacon_context_t *context);
void beacon_context_registerFormRenderingPrimitives(beacon_context_t *context);
void beacon_context_registerFontFacePrimitives(beacon_context_t *context);
void beacon_context_registerWindowSystemPrimitives(beacon_context_t *context);
void beacon_context_registerSourceCodePrimitives(beacon_context_t *context);
void beacon_context_registerParseTreeCompilationPrimitives(beacon_context_t *context);
void beacon_context_registerLinearAlgebraPrimitives(beacon_context_t *context);

static size_t beacon_context_computeBehaviorSlotCount(beacon_context_t *context, beacon_Behavior_t *behavior)
{
    if(!behavior)
        return 0;

    size_t slotCount = behavior->slots ? ((beacon_ObjectHeader_t*)behavior->slots)->slotCount : 0;
    return slotCount + beacon_context_computeBehaviorSlotCount(context, behavior->superclass);
}

static beacon_Behavior_t *beacon_context_createClassAndMetaclass(beacon_context_t *context, beacon_Behavior_t *superclassBehavior, const char *name, size_t instanceSize, beacon_ObjectKind_t objectKind, ...)
{
    BeaconAssert(context, instanceSize >= sizeof(beacon_ObjectHeader_t));

    size_t slotCount = 0; 
    size_t metaSlotCount = 0;
    if(superclassBehavior)
    {
        slotCount = beacon_context_computeBehaviorSlotCount(context, superclassBehavior);
        metaSlotCount = beacon_context_computeBehaviorSlotCount(context, ((beacon_ObjectHeader_t*)superclassBehavior)->behavior);
    }

    va_list varNames;
    va_start(varNames, objectKind);
    beacon_ArrayList_t *classVarNames = beacon_ArrayList_new(context);
    beacon_ArrayList_t *metaClassVarNames = beacon_ArrayList_new(context);
    const char *nextVarName = va_arg(varNames, const char *);
    bool parsingMetaVariable = false;
    while(nextVarName)
    {
        if(!strcmp(nextVarName, "__Meta__"))
        {
            parsingMetaVariable = true;
            nextVarName = va_arg(varNames, const char *);
            continue;
        }

        beacon_Symbol_t *varNameSymbol = beacon_internCString(context, nextVarName);
        beacon_Slot_t *varSlot = beacon_allocateObjectWithBehavior(context->heap, context->classes.slotClass, sizeof(beacon_Slot_t), BeaconObjectKindPointers);
        varSlot->name = (beacon_oop_t)varNameSymbol;
        if(parsingMetaVariable)
        {
            varSlot->index = beacon_encodeSmallInteger(++metaSlotCount);
            beacon_ArrayList_add(context, metaClassVarNames, (beacon_oop_t)varSlot);
        }
        else
        {
            varSlot->index = beacon_encodeSmallInteger(++slotCount);
            beacon_ArrayList_add(context, classVarNames, (beacon_oop_t)varSlot);
        }
        
        nextVarName = va_arg(varNames, const char *);
    }

    va_end(varNames);

    size_t metaClassExtraSize  = beacon_ArrayList_size(metaClassVarNames);
    beacon_Metaclass_t *metaclass = beacon_allocateObjectWithBehavior(context->heap, context->classes.metaclassClass, sizeof(beacon_Metaclass_t), BeaconObjectKindPointers);
    metaclass->super.super.instSize = beacon_encodeSmallInteger(sizeof(beacon_Class_t) - sizeof(beacon_ObjectHeader_t) + metaClassExtraSize*sizeof(beacon_oop_t));
    metaclass->super.super.objectKind = beacon_encodeSmallInteger(BeaconObjectKindPointers);
    metaclass->super.super.slots = (beacon_oop_t)beacon_ArrayList_asArray(context, metaClassVarNames);
    if(superclassBehavior)
    {
        metaclass->super.super.instSize = beacon_encodeSmallInteger(
            beacon_decodeSmallInteger(superclassBehavior->super.super.header.behavior->instSize)
            + metaClassExtraSize * sizeof(beacon_oop_t));
    }

    beacon_Class_t *clazz = beacon_allocateObjectWithBehavior(context->heap, (beacon_Behavior_t*)metaclass, sizeof(beacon_ObjectHeader_t) + beacon_decodeSmallInteger(metaclass->super.super.instSize), BeaconObjectKindPointers);
    metaclass->thisClass = clazz;
    if(name)
        clazz->name = beacon_internCString(context, name);
    clazz->super.super.instSize = beacon_encodeSmallInteger(instanceSize - sizeof(beacon_ObjectHeader_t));
    clazz->super.super.objectKind = beacon_encodeSmallInteger(objectKind);
    clazz->super.super.slots = (beacon_oop_t)beacon_ArrayList_asArray(context, classVarNames);
    clazz->subclasses = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t), BeaconObjectKindPointers);

    if(superclassBehavior)
    {
        clazz->super.super.superclass = superclassBehavior;
        metaclass->super.super.superclass = superclassBehavior->super.super.header.behavior;

        beacon_Class_t *superclazz = (beacon_Class_t*)superclassBehavior;
        superclazz->subclasses = (beacon_oop_t)beacon_Array_copyWith(context, (beacon_Array_t*)superclazz->subclasses, (beacon_oop_t)clazz);
    }

    return &clazz->super.super;
}

static void beacon_context_fixEarlyMetaclass(beacon_context_t *context, beacon_Behavior_t *earlyClassBehavior)
{
    beacon_Behavior_t *earlyMetaClass = earlyClassBehavior->super.super.header.behavior;
    earlyMetaClass->super.super.header.behavior = context->classes.metaclassClass;
}

static void beacon_context_fixEarlyObjectClasses(beacon_context_t *context, beacon_Behavior_t *earlyClassBehavior)
{
    if(!earlyClassBehavior->slots)
        return;

    beacon_Class_t *class = (beacon_Class_t*)earlyClassBehavior;

    ((beacon_ObjectHeader_t*)earlyClassBehavior->slots)->behavior = context->classes.arrayClass;
    ((beacon_ObjectHeader_t*)class->name)->behavior = context->classes.symbolClass;
    ((beacon_ObjectHeader_t*)class->subclasses)->behavior = context->classes.arrayClass;
}

static void beacon_context_createBaseClassHierarchy(beacon_context_t *context)
{
    // Meta-circular hierarchy.
    context->classes.protoObjectClass = beacon_context_createClassAndMetaclass(context, NULL, "ProtoObject", sizeof(beacon_ProtoObject_t), BeaconObjectKindPointers, NULL);
    context->classes.objectClass = beacon_context_createClassAndMetaclass(context, context->classes.protoObjectClass, "Object", sizeof(beacon_Object_t), BeaconObjectKindPointers, NULL);
    context->classes.behaviorClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Behavior", sizeof(beacon_Behavior_t), BeaconObjectKindPointers,
        "superclass", "methodDict", "instSize", "objectKind", "slots", NULL);
    context->classes.classDescriptionClass = beacon_context_createClassAndMetaclass(context, context->classes.behaviorClass, "ClassDescription", sizeof(beacon_ClassDescription_t), BeaconObjectKindPointers,
        "protocols", NULL);
    context->classes.metaclassClass = beacon_context_createClassAndMetaclass(context, context->classes.classDescriptionClass, "Metaclass", sizeof(beacon_Metaclass_t), BeaconObjectKindPointers,
        "thisClass", NULL);
    context->classes.slotClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Slot", sizeof(beacon_Slot_t), BeaconObjectKindPointers,
        "name", "index", NULL);
    
    beacon_context_fixEarlyMetaclass(context, context->classes.protoObjectClass);
    beacon_context_fixEarlyMetaclass(context, context->classes.objectClass);
    beacon_context_fixEarlyMetaclass(context, context->classes.behaviorClass);
    beacon_context_fixEarlyMetaclass(context, context->classes.classDescriptionClass);
    beacon_context_fixEarlyMetaclass(context, context->classes.metaclassClass);
    context->classes.classClass = beacon_context_createClassAndMetaclass(context, context->classes.classDescriptionClass, "Class", sizeof(beacon_Class_t), BeaconObjectKindPointers,
        "subclasses", "name", NULL);
    context->classes.protoObjectClass->super.super.header.behavior->superclass = context->classes.classClass;

    // Normal class orders
    context->classes.collectionClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Collection", sizeof(beacon_Collection_t), BeaconObjectKindPointers, NULL);
    context->classes.hashedCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.collectionClass, "HashedCollection", sizeof(beacon_HashedCollection_t), BeaconObjectKindPointers,
        "tally", "array", NULL);
    context->classes.associationClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Association", sizeof(beacon_Association_t), BeaconObjectKindPointers,
        "key", "value", NULL);
    context->classes.dictionaryClass = beacon_context_createClassAndMetaclass(context, context->classes.hashedCollectionClass, "Dictionary", sizeof(beacon_Dictionary_t), BeaconObjectKindPointers, NULL);
    context->classes.methodDictionaryClass = beacon_context_createClassAndMetaclass(context, context->classes.dictionaryClass, "MethodDictionary", sizeof(beacon_MethodDictionary_t), BeaconObjectKindPointers, NULL);
    context->classes.sequenceableCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.collectionClass, "SequenceableCollection", sizeof(beacon_SequenceableCollection_t), BeaconObjectKindPointers, NULL);
    context->classes.arrayedCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.sequenceableCollectionClass, "ArrayedCollection", sizeof(beacon_ArrayedCollection_t), BeaconObjectKindPointers, NULL);
    context->classes.arrayClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "Array", sizeof(beacon_Array_t), BeaconObjectKindPointers, NULL);
    context->classes.symbolClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "Symbol", sizeof(beacon_Symbol_t), BeaconObjectKindBytes, NULL);

    beacon_context_fixEarlyObjectClasses(context, context->classes.protoObjectClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.objectClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.behaviorClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.classDescriptionClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.metaclassClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.slotClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.collectionClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.hashedCollectionClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.associationClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.dictionaryClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.methodDictionaryClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.sequenceableCollectionClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.arrayedCollectionClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.arrayClass);
    beacon_context_fixEarlyObjectClasses(context, context->classes.symbolClass);
    
    context->classes.byteArrayClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "ByteArray", sizeof(beacon_ByteArray_t), BeaconObjectKindBytes, NULL);
    context->classes.stringClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "String", sizeof(beacon_String_t), BeaconObjectKindBytes, NULL);
    context->classes.externalAddressClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "ExternalAddress", sizeof(beacon_ExternalAddress_t), BeaconObjectKindBytes, NULL);

    context->classes.arrayListClass = beacon_context_createClassAndMetaclass(context, context->classes.sequenceableCollectionClass, "ArrayList", sizeof(beacon_ArrayList_t), BeaconObjectKindPointers,
    "array", "size", "capacity", NULL);
    context->classes.byteArrayListClass = beacon_context_createClassAndMetaclass(context, context->classes.sequenceableCollectionClass, "ByteArrayList", sizeof(beacon_ByteArrayList_t), BeaconObjectKindPointers,
    "array", "size", "capacity", NULL);

    context->classes.booleanClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Boolean", sizeof(beacon_Boolean_t), BeaconObjectKindPointers, NULL);
    context->classes.trueClass = beacon_context_createClassAndMetaclass(context, context->classes.booleanClass, "True", sizeof(beacon_True_t), BeaconObjectKindPointers, NULL);
    context->classes.falseClass = beacon_context_createClassAndMetaclass(context, context->classes.booleanClass, "False", sizeof(beacon_False_t), BeaconObjectKindPointers, NULL);
    context->classes.undefinedObjectClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "UndefinedObject", sizeof(beacon_UndefinedObject_t), BeaconObjectKindImmediate, NULL);

    context->classes.magnitudeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Magnitude", sizeof(beacon_Magnitude_t), BeaconObjectKindPointers, NULL);
    context->classes.numberClass = beacon_context_createClassAndMetaclass(context, context->classes.magnitudeClass, "Number", sizeof(beacon_Number_t), BeaconObjectKindPointers, NULL);
    context->classes.integerClass = beacon_context_createClassAndMetaclass(context, context->classes.numberClass, "Integer", sizeof(beacon_Integer_t), BeaconObjectKindPointers, NULL);
    context->classes.smallIntegerClass = beacon_context_createClassAndMetaclass(context, context->classes.integerClass, "SmallInteger", sizeof(beacon_SmallInteger_t), BeaconObjectKindImmediate, NULL);
    context->classes.floatClass = beacon_context_createClassAndMetaclass(context, context->classes.numberClass, "Float", sizeof(beacon_Float_t), BeaconObjectKindBytes, NULL);
    context->classes.smallFloatClass = beacon_context_createClassAndMetaclass(context, context->classes.floatClass, "SmallFloat", sizeof(beacon_SmallFloat_t), BeaconObjectKindImmediate, NULL);

    context->classes.characterClass = beacon_context_createClassAndMetaclass(context, context->classes.magnitudeClass, "Character", sizeof(beacon_Character_t), BeaconObjectKindImmediate, NULL);

    context->classes.nativeCodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "NativeCode", sizeof(beacon_NativeCode_t), BeaconObjectKindBytes, NULL);
    context->classes.bytecodeCodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "BytecodeCode", sizeof(beacon_BytecodeCode_t), BeaconObjectKindPointers,
        "argumentCount", "temporaryCount", "captureCount", "literals", "bytecodes", NULL);
    context->classes.bytecodeCodeBuilderClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "BytecodeCodeBuilder", sizeof(beacon_BytecodeCodeBuilder_t), BeaconObjectKindPointers,
        "arguments", "temporaries", "literals", "bytecodes", NULL);
    context->classes.compiledCodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "CompiledCode", sizeof(beacon_CompiledCode_t), BeaconObjectKindPointers,
        "argumentCount",  "nativeImplementation", "bytecodeImplementation", "sourcePosition", NULL);
    context->classes.compiledBlockClass = beacon_context_createClassAndMetaclass(context, context->classes.compiledCodeClass, "CompiledBlock", sizeof(beacon_CompiledBlock_t), BeaconObjectKindPointers,
        "captureCount", NULL);
    context->classes.compiledMethodClass = beacon_context_createClassAndMetaclass(context, context->classes.compiledCodeClass, "CompiledMethod", sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers,
        "name", NULL);
    context->classes.blockClosureClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "BlockClosure", sizeof(beacon_BlockClosure_t), BeaconObjectKindPointers,
        "code", "captures", NULL);
    context->classes.messageClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Message", sizeof(beacon_Message_t), BeaconObjectKindPointers,
        "selector", "arguments", NULL);

    context->classes.sourceCodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "SourceCode", sizeof(beacon_SourceCode_t), BeaconObjectKindPointers,
        "directory", "name", "text", "textSize", NULL);
    context->classes.sourcePositionClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "SourcePosition", sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers,
        "sourceCode", "startIndex", "endIndex", "startLine", "endLine", "startColumn", "endColumn", NULL);
    context->classes.scannerTokenClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "ScannerToken", sizeof(beacon_ScannerToken_t), BeaconObjectKindPointers,
        "kind", "sourcePosition", "errorMessage", "textPosition", "textSize", NULL);

    context->classes.parseTreeNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "ParseTreeNode", sizeof(beacon_ParseTreeNode_t), BeaconObjectKindPointers,
        "sourcePosition", NULL);
    context->classes.parseTreeErrorNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeErrorNode", sizeof(beacon_ParseTreeErrorNode_t), BeaconObjectKindPointers,
        "errorMessage", "innerNode", NULL);
    context->classes.parseTreeLiteralNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeLiteralNode", sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers,
        "value", NULL);
    context->classes.parseTreeIdentifierReferenceNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeIdentifierReferenceNode", sizeof(beacon_ParseTreeIdentifierReferenceNode_t), BeaconObjectKindPointers,
        "identifier", NULL);
    context->classes.parseTreeMessageSendNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeMessageSendNode", sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers,
        "receiver", "selector", "arguments", NULL);
    context->classes.parseTreeMessageCascadeNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeMessageCascadeNode", sizeof(beacon_ParseTreeMessageCascadeNode_t), BeaconObjectKindPointers,
        "receiver", "cascadedMessages", NULL);
    context->classes.parseTreeCascadedMessageNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeCascadedMessageNpde", sizeof(beacon_ParseTreeCascadedMessageNode_t), BeaconObjectKindPointers,
        "selector", "arguments", NULL);
    context->classes.parseTreeSequenceNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeSequenceNode", sizeof(beacon_ParseTreeSequenceNode_t), BeaconObjectKindPointers,
        "elements", NULL);
    context->classes.parseTreeReturnNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeReturnNode", sizeof(beacon_ParseTreeReturnNode_t), BeaconObjectKindPointers,
        "expression", NULL);
    context->classes.parseTreeAssignmentNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeAssignmentNode", sizeof(beacon_ParseTreeAssignment_t), BeaconObjectKindPointers,
        "storage", "valueToStore", NULL);
    context->classes.parseTreeArrayNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeArrayNode", sizeof(beacon_ParseTreeArrayNode_t), BeaconObjectKindPointers,
        "elements", NULL);
    context->classes.parseTreeByteArrayNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeByteArrayNode", sizeof(beacon_ParseTreeByteArrayNode_t), BeaconObjectKindPointers,
        "elements", NULL);
    context->classes.parseTreeLiteralArrayNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeLiteralArrayNode", sizeof(beacon_ParseTreeLiteralArrayNode_t), BeaconObjectKindPointers,
        "elements", NULL);
    context->classes.parseTreeArgumentDefinitionNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeArgumentDefinitionNode", sizeof(beacon_ParseTreeArgumentDefinitionNode_t), BeaconObjectKindPointers,
        "name", NULL);
    context->classes.parseTreeLocalVariableDefinitionNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeLocalVariableDefinitionNode", sizeof(beacon_ParseTreeLocalVariableDefinitionNode_t), BeaconObjectKindPointers,
        "name", NULL);
    context->classes.parseTreeBlockClosureNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeBlockClosureNode", sizeof(beacon_ParseTreeBlockClosureNode_t), BeaconObjectKindPointers,
        "arguments", "localVariables", "expression", NULL);
    context->classes.parseTreeAddMethodNodeClass = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeAddMethod", sizeof(beacon_ParseTreeAddMethod_t), BeaconObjectKindPointers,
        "behavior", "method", NULL);
    context->classes.parseTreeMethodNode = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeMethodNode", sizeof(beacon_ParseTreeMethodNode_t), BeaconObjectKindPointers,
        "selector", "arguments", "localVariables", "expression",NULL);
    context->classes.parseTreeWorkspaceScriptNode = beacon_context_createClassAndMetaclass(context, context->classes.parseTreeNodeClass, "ParseTreeWorkspaceScriptNode", sizeof(beacon_ParseTreeWorkspaceScriptNode_t), BeaconObjectKindPointers,
        "localVariables", "expression", NULL);

    context->classes.abstractCompilationEnvironmentClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "AbstractCompilationEnvironment", sizeof(beacon_AbstractCompilationEnvironment_t), BeaconObjectKindPointers, NULL);
    context->classes.emptyCompilationEnvironmentClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractCompilationEnvironmentClass, "EmptyCompilationEnvironment", sizeof(beacon_EmptyCompilationEnvironment_t), BeaconObjectKindPointers, NULL);
    context->classes.systemCompilationEnvironmentClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractCompilationEnvironmentClass, "SystemCompilationEnvironment", sizeof(beacon_SystemCompilationEnvironment_t), BeaconObjectKindPointers,
        "parent", "systemDictionary", NULL);
    context->classes.fileCompilationEnvironmentClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractCompilationEnvironmentClass, "FileCompilationEnvironment", sizeof(beacon_FileCompilationEnvironment_t), BeaconObjectKindPointers,
        "parent", "dictionary", NULL);
    context->classes.lexicalCompilationEnvironmentClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractCompilationEnvironmentClass, "LexicalCompilationEnvironment", sizeof(beacon_LexicalCompilationEnvironment_t), BeaconObjectKindPointers,
        "parent", "dictionary", NULL);
    context->classes.blockClosureCompilationEnvironmentClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractCompilationEnvironmentClass, "BlockClosureCompilationEnvironment", sizeof(beacon_BlockClosureCompilationEnvironment_t), BeaconObjectKindPointers,
        "parent", "captureList", "dictionary", NULL);

    context->classes.methodCompilationEnvironmentClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractCompilationEnvironmentClass, "MethodCompilationEnvironment", sizeof(beacon_MethodCompilationEnvironment_t), BeaconObjectKindPointers,
        "parent", "dictionary", NULL);
    context->classes.behaviorCompilationEnvironmentClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractCompilationEnvironmentClass, "BehaviorCompilationEnvironment", sizeof(beacon_BehaviorCompilationEnvironment_t), BeaconObjectKindPointers,
        "parent", "behavior", NULL);

    context->classes.exceptionClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Exception", sizeof(beacon_Exception_t), BeaconObjectKindPointers,
        "messageText", NULL);
    context->classes.errorClass = beacon_context_createClassAndMetaclass(context, context->classes.exceptionClass, "Error", sizeof(beacon_Error_t), BeaconObjectKindPointers, NULL);
    context->classes.assertionFailureClass = beacon_context_createClassAndMetaclass(context, context->classes.errorClass, "AssertionFailure", sizeof(beacon_AssertionFailure_t), BeaconObjectKindPointers, NULL);
    context->classes.messageNotUnderstoodClass = beacon_context_createClassAndMetaclass(context, context->classes.errorClass, "MessageNotUnderstood", sizeof(beacon_MessageNotUnderstood_t), BeaconObjectKindPointers,
        "message", "receiver", "reachedDefaultHandler", NULL);
    context->classes.nonBooleanReceiverClass = beacon_context_createClassAndMetaclass(context, context->classes.errorClass, "NonBooleanReceiver", sizeof(beacon_NonBooleanReceiver_t), BeaconObjectKindPointers, NULL);
    context->classes.unhandledExceptionClass = beacon_context_createClassAndMetaclass(context, context->classes.exceptionClass, "UnhandledException", sizeof(beacon_UnhandledException_t), BeaconObjectKindPointers, NULL);
    context->classes.unhandledErrorClass = beacon_context_createClassAndMetaclass(context, context->classes.unhandledExceptionClass, "UnhandledError", sizeof(beacon_UnhandledError_t), BeaconObjectKindPointers, NULL);

    context->classes.weakTombstoneClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "WeakTombstone", sizeof(beacon_WeakTombstone_t), BeaconObjectKindPointers, NULL);

    context->classes.streamClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Stream", sizeof(beacon_Stdio_t), BeaconObjectKindPointers, NULL);
    context->classes.abstractBinaryFileStreamClass = beacon_context_createClassAndMetaclass(context, context->classes.streamClass, "AbstractBinaryFileStream", sizeof(beacon_Stdio_t), BeaconObjectKindPointers,
        "handle", NULL);
    context->classes.stdioClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Stdio", sizeof(beacon_Stdio_t), BeaconObjectKindPointers, NULL);
    context->classes.stdioStreamClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractBinaryFileStreamClass, "StdioStream", sizeof(beacon_Stdio_t), BeaconObjectKindPointers, NULL);

    context->classes.pointClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Point", sizeof(beacon_Point_t), BeaconObjectKindPointers,
        "x", "y", NULL);
    context->classes.colorClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Color", sizeof(beacon_Color_t), BeaconObjectKindPointers,
        "r", "g", "b", "a", NULL);
    context->classes.rectangleClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Rectangle", sizeof(beacon_Rectangle_t), BeaconObjectKindPointers,
        "origin", "corner", NULL);

    context->classes.formClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Form", sizeof(beacon_Form_t), BeaconObjectKindPointers,
        "bits", "width", "height", "depth", "pitch", "textureHandle", NULL);
    context->classes.fontClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Font", sizeof(beacon_Font_t), BeaconObjectKindPointers,
        "rawData", "__Meta__",
        "DejaVuSans", "DejaVuSansSmallFace", "DejaVuSansSmallHiDpiFace",
        "DejaVuSansMono", "DejaVuSansMonoSmallFace", "DejaVuSansMonoSmallHiDpiFace", NULL);
    context->classes.fontFaceClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "FontFace", sizeof(beacon_FontFace_t), BeaconObjectKindPointers,
        "charData", "height", "ascent", "descent", "linegap", "atlasForm", "hiDpiScaled", NULL);
    context->classes.formRenderingElementClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "FormRenderingElement", sizeof(beacon_FormRenderingElement_t), BeaconObjectKindPointers,
        "borderRoundRadius", "borderSize", "rectangle", NULL);
    context->classes.formSolidRectangleRenderingElementClass = beacon_context_createClassAndMetaclass(context, context->classes.formRenderingElementClass, "FormSolidRectangleRenderingElement", sizeof(beacon_FormSolidRectangleRenderingElement_t), BeaconObjectKindPointers,
        "color", "borderColor", NULL);
    context->classes.formHorizontalGradientRenderingElementClass = beacon_context_createClassAndMetaclass(context, context->classes.formRenderingElementClass, "FormHorizontalGradientRenderingElement", sizeof(beacon_FormHorizontalGradientRenderingElement_t), BeaconObjectKindPointers,
        "startColor", "endColor", "borderColor", NULL);
    context->classes.formVerticalGradientRenderingElementClass = beacon_context_createClassAndMetaclass(context, context->classes.formRenderingElementClass, "FormHorizontalGradientRenderingElement", sizeof(beacon_FormVerticalGradientRenderingElement_t), BeaconObjectKindPointers,
        "startColor", "endColor", "borderColor", NULL);
    context->classes.formTextRenderingElementClass = beacon_context_createClassAndMetaclass(context, context->classes.formRenderingElementClass, "FormTextRenderingElement", sizeof(beacon_FormTextRenderingElement_t), BeaconObjectKindPointers,
        "text", "color", "fontFace", NULL);

    context->classes.windowClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Window", sizeof(beacon_Window_t), BeaconObjectKindPointers,
        "width", "height", "handle", "rendererHandle", "textureHandle", "textureWidth", "textureHeight", "drawingForm",
        "useAcceleratedRendering", "swapChainHandle", NULL);

    context->classes.windowEventClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "WindowEvent", sizeof(beacon_WindowEvent_t), BeaconObjectKindPointers, NULL);
    context->classes.windowExposeEventClass = beacon_context_createClassAndMetaclass(context, context->classes.windowEventClass, "WindowExposeEvent", sizeof(beacon_WindowExposeEvent_t), BeaconObjectKindPointers,
        "x", "y", "width", "height",NULL);
    context->classes.windowMouseButtonEventClass = beacon_context_createClassAndMetaclass(context, context->classes.windowEventClass, "WindowMouseButtonEvent", sizeof(beacon_WindowMouseButtonEvent_t), BeaconObjectKindPointers,
        "button", "x", "y", NULL);
    context->classes.windowMouseMotionEventClass = beacon_context_createClassAndMetaclass(context, context->classes.windowEventClass, "WindowMouseMotionEvent", sizeof(beacon_WindowMouseMotionEvent_t), BeaconObjectKindPointers,
        "buttons", "x", "y", "xrel", "yrel", NULL);
    context->classes.windowMouseWheelEventClass = beacon_context_createClassAndMetaclass(context, context->classes.windowEventClass, "WindowMouseWheelEvent", sizeof(beacon_WindowMouseWheelEvent_t), BeaconObjectKindPointers,
        "x", "y", "scrollX", "scrollY", NULL);
    context->classes.windowKeyboardEventClass = beacon_context_createClassAndMetaclass(context, context->classes.windowEventClass, "WindowKeyboardEvent", sizeof(beacon_WindowMouseButtonEvent_t), BeaconObjectKindPointers,
        "scancode", "symbol", "modstate", NULL);
    context->classes.windowTextInputEventClass = beacon_context_createClassAndMetaclass(context, context->classes.windowEventClass, "WindowTextInputEvent", sizeof(beacon_WindowTextInputEvent_t), BeaconObjectKindPointers,
        "text", NULL);

    context->classes.testAsserterClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "TestAsserter", sizeof(beacon_TestAsserter_t), BeaconObjectKindPointers, NULL);
    context->classes.testCaseClass = beacon_context_createClassAndMetaclass(context, context->classes.testAsserterClass, "TestCase", sizeof(beacon_TestCase_t), BeaconObjectKindPointers, NULL);

    context->classes.abstractPrimitiveTensorClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "AbstractPrimitiveTensor", sizeof(beacon_AbstractPrimitiveTensor_t), BeaconObjectKindBytes,  NULL);
    context->classes.abstractPrimitiveMatrixClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractPrimitiveTensorClass, "AbstractPrimitiveMatrix", sizeof(beacon_AbstractPrimitiveMatrix_t), BeaconObjectKindBytes,  NULL);
    context->classes.matrix2x2Class = beacon_context_createClassAndMetaclass(context, context->classes.abstractPrimitiveMatrixClass, "Matrix2x2", sizeof(beacon_Matrix2x2_t), BeaconObjectKindBytes,  NULL);
    context->classes.matrix3x3Class = beacon_context_createClassAndMetaclass(context, context->classes.abstractPrimitiveMatrixClass, "Matrix3x3", sizeof(beacon_Matrix3x3_t), BeaconObjectKindBytes,  NULL);
    context->classes.matrix4x4Class = beacon_context_createClassAndMetaclass(context, context->classes.abstractPrimitiveMatrixClass, "Matrix4x4", sizeof(beacon_Matrix4x4_t), BeaconObjectKindBytes,  NULL);
    context->classes.abstractPrimitiveVectorClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractPrimitiveTensorClass, "AbstractPrimitiveVector", sizeof(beacon_AbstractPrimitiveVector_t), BeaconObjectKindBytes,  NULL);
    context->classes.vector2Class = beacon_context_createClassAndMetaclass(context, context->classes.abstractPrimitiveVectorClass, "Vector2", sizeof(beacon_Vector2_t), BeaconObjectKindBytes,  NULL);
    context->classes.vector3Class = beacon_context_createClassAndMetaclass(context, context->classes.abstractPrimitiveVectorClass, "Vector3", sizeof(beacon_Vector3_t), BeaconObjectKindBytes,  NULL);
    context->classes.vector4Class = beacon_context_createClassAndMetaclass(context, context->classes.abstractPrimitiveVectorClass, "Vector4", sizeof(beacon_Vector4_t), BeaconObjectKindBytes,  NULL);

    context->classes.complexClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Complex", sizeof(beacon_Complex_t), BeaconObjectKindBytes,  NULL);
    context->classes.quaternionClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Quaternion", sizeof(beacon_Quaternion_t), BeaconObjectKindBytes,  NULL);
    
    context->classes.agpuClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "AGPU", sizeof(beacon_AGPU_t), BeaconObjectKindBytes, NULL);
    context->classes.agpuSwapChainClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "AGPUSwapchain", sizeof(beacon_AGPUSwapChain_t), BeaconObjectKindBytes, NULL);
    context->classes.agpuTextureHandleClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "AGPUTextureHandle", sizeof(beacon_AGPUTextureHandle_t), BeaconObjectKindBytes, NULL);
    context->classes.agpuWindowRendererClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "AGPUWindowRenderer", sizeof(beacon_AGPUWindowRenderer_t), BeaconObjectKindBytes, NULL);
}

void beacon_context_createImportantRoots(beacon_context_t *context)
{
    context->roots.trueValue = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, context->classes.trueClass, sizeof(beacon_True_t), BeaconObjectKindPointers);
    context->roots.falseValue = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, context->classes.falseClass, sizeof(beacon_False_t), BeaconObjectKindPointers);
    context->roots.doesNotUnderstandSelector = (beacon_oop_t)beacon_internCString(context, "doesNotUnderstand:");
    context->roots.compileWithEnvironmentAndBytecodeBuilderSelector = (beacon_oop_t)beacon_internCString(context, "compileWithEnvironment:andBytecodeBuilder:");
    context->roots.compileInlineBlockWithArgumentsWithEnvironmentAndBytecodeBuilderSelector = (beacon_oop_t)beacon_internCString(context, "compileInlineBlockWithArguments:environment:andBytecodeBuilder:");
    
    context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector = (beacon_oop_t)beacon_internCString(context, "lookupSymbolRecursively:withBytecodeBuilder:");
    context->roots.lookupSymbolRecursivelySelector = (beacon_oop_t)beacon_internCString(context, "lookupSymbolRecursively:");
    context->roots.evaluateWithEnvironmentSelector = (beacon_oop_t)beacon_internCString(context, "evaluateWithEnvironment:");
    context->roots.addCompiledMethodSelector = (beacon_oop_t)beacon_internCString(context, "addCompiledMethod:");

    context->roots.emptyArray = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t), BeaconObjectKindPointers);
    context->roots.weakTombstone = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, context->classes.weakTombstoneClass, sizeof(beacon_WeakTombstone_t), BeaconObjectKindPointers);

    {
        beacon_StdioStream_t *stdioStdin = beacon_allocateObjectWithBehavior(context->heap, context->classes.stdioStreamClass, sizeof(beacon_StdioStream_t), BeaconObjectKindPointers);
        stdioStdin->super.handle = beacon_encodeSmallInteger(STDIN_FILENO);
        context->roots.stdinStream = (beacon_oop_t)stdioStdin;
    }

    {
        beacon_StdioStream_t *stdioStdout = beacon_allocateObjectWithBehavior(context->heap, context->classes.stdioStreamClass, sizeof(beacon_StdioStream_t), BeaconObjectKindPointers);
        stdioStdout->super.handle = beacon_encodeSmallInteger(STDOUT_FILENO);
        context->roots.stdoutStream = (beacon_oop_t)stdioStdout;
    }

    {
        beacon_StdioStream_t *stderrStdout = beacon_allocateObjectWithBehavior(context->heap, context->classes.stdioStreamClass, sizeof(beacon_StdioStream_t), BeaconObjectKindPointers);
        stderrStdout->super.handle = beacon_encodeSmallInteger(STDERR_FILENO);
        context->roots.stdoutStream = (beacon_oop_t)stderrStdout;
    }

    {
        context->roots.ifTrueSelector = (beacon_oop_t)beacon_internCString(context, "ifTrue:");
        context->roots.ifFalseSelector = (beacon_oop_t)beacon_internCString(context, "ifFalse:");
        context->roots.ifTrueIfFalseSelector = (beacon_oop_t)beacon_internCString(context, "ifTrue:ifFalse:");
        context->roots.ifFalseIfTrueSelector = (beacon_oop_t)beacon_internCString(context, "ifFalse:ifTrue:");
        context->roots.andSelector = (beacon_oop_t)beacon_internCString(context, "and:");
        context->roots.orSelector = (beacon_oop_t)beacon_internCString(context, "or:");
        context->roots.whileTrueSelector = (beacon_oop_t)beacon_internCString(context, "whileTrue");
        context->roots.whileFalseSelector = (beacon_oop_t)beacon_internCString(context, "whileFalse");
        context->roots.whileTrueDoSelector = (beacon_oop_t)beacon_internCString(context, "whileTrue:");
        context->roots.whileFalseDoSelector = (beacon_oop_t)beacon_internCString(context, "whileFalse:");
        context->roots.doWhileTrueSelector = (beacon_oop_t)beacon_internCString(context, "do:whileTrue:");
        context->roots.doWhileFalseSelector = (beacon_oop_t)beacon_internCString(context, "do:whileFalse:");
        context->roots.toDoSelector = (beacon_oop_t)beacon_internCString(context, "to:do:");
    }

    {
        context->roots.plusSelector = (beacon_oop_t)beacon_internCString(context, "+");
        context->roots.lessOrEqualsSelector = (beacon_oop_t)beacon_internCString(context, "<=");
    }

    context->roots.windowHandleMap = beacon_MethodDictionary_new(context);
    context->roots.agpuCommon = beacon_allocateObjectWithBehavior(context->heap, context->classes.agpuClass, sizeof(beacon_AGPU_t), BeaconObjectKindBytes);
    context->roots.agpuCommon->debugLayerEnabled = true;
}

void beacon_context_createSystemDictionary(beacon_context_t *context)
{
    if(!context->roots.systemDictionary)
        context->roots.systemDictionary = beacon_MethodDictionary_new(context);
    
    beacon_Class_t **globalClassList = (beacon_Class_t **)&context->classes;
    size_t globalClassListSize = sizeof(context->classes) / sizeof(beacon_Class_t *);
    for(size_t i = 0; i < globalClassListSize; ++i)
    {
        beacon_Class_t *globalClass = globalClassList[i];
        if(globalClass->name)
        {
            beacon_context_fixEarlyObjectClasses(context, &globalClass->super.super);
            beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, globalClass->name, (beacon_oop_t)globalClass);
        }
    }

    beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, beacon_internCString(context, "nil"), context->roots.nilValue);
    beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, beacon_internCString(context, "true"), context->roots.trueValue);
    beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, beacon_internCString(context, "false"), context->roots.falseValue);
    beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, beacon_internCString(context, "SystemDictionary"), (beacon_oop_t)context->roots.systemDictionary);
}

void beacon_context_registerBasicPrimitives(beacon_context_t *context)
{
    beacon_context_registerObjectBasicPrimitives(context);
    beacon_context_registerArrayListPrimitive(context);
    beacon_context_registerDictionaryPrimitives(context);
    beacon_context_registerExceptionPrimitives(context);
    beacon_context_registerAgpuRenderingPrimitives(context);
    beacon_context_registerFormRenderingPrimitives(context);
    beacon_context_registerFontFacePrimitives(context);
    beacon_context_registerWindowSystemPrimitives(context);
    beacon_context_registerSourceCodePrimitives(context);
    beacon_context_registerParseTreeCompilationPrimitives(context);
    beacon_context_registerLinearAlgebraPrimitives(context);
}

beacon_context_t *beacon_context_new(void)
{
    beacon_context_t *context = calloc(1, sizeof(beacon_context_t));
    context->heap = beacon_createMemoryHeap(context);
    context->roots.internedSymbolSet = beacon_allocateObject(context->heap, sizeof(beacon_InternedSymbolSet_t), BeaconObjectKindPointers);
    context->roots.internedSymbolSet->super.array = beacon_allocateObject(context->heap, sizeof(beacon_Array_t) + sizeof(beacon_oop_t)*1024, BeaconObjectKindPointers);
    context->roots.internedSymbolSet->super.tally = beacon_encodeSmallInteger(0);
    beacon_context_createBaseClassHierarchy(context);
    beacon_context_createImportantRoots(context);
    beacon_context_createSystemDictionary(context);
    beacon_context_registerBasicPrimitives(context);
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

uint32_t beacon_computeIdentityHash(beacon_oop_t oop)
{
    // See https://en.wikipedia.org/wiki/Linear_congruential_generator [February, 2025]
    return (uint32_t) (oop*1664525 + 1013904223);
}

beacon_String_t *beacon_importCString(beacon_context_t *context, const char *string)
{
    size_t stringSize = strlen(string);
    beacon_String_t *importedString = beacon_allocateObjectWithBehavior(context->heap, context->classes.stringClass, sizeof(beacon_String_t) + stringSize, BeaconObjectKindBytes);
    memcpy(importedString->data, string, stringSize);
    return importedString;
}

beacon_String_t *beacon_importStringWithSize(beacon_context_t *context, size_t stringSize, const char *string)
{
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
                            && !memcmp(storedSymbol->data, string, stringSize)))
            return i;
    }

    for(size_t i = 0; i < naturalSlot; ++i)
    {
        beacon_Symbol_t *storedSymbol = (beacon_Symbol_t *)storage->elements[i];
        if(beacon_isNil((beacon_oop_t)storedSymbol) || (storedSymbol->super.super.super.super.super.header.slotCount == stringSize
                            && !memcmp(storedSymbol->data, string, stringSize)))
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
        BeaconAssert(context, symbolSetPosition >= 0);
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

beacon_Behavior_t *beacon_getClass(beacon_context_t *context, beacon_oop_t receiver)
{
    beacon_oop_t tagBits = receiver & 7;
    if(!receiver || tagBits != 0)
    {
        switch(tagBits)
        {
        case 0: return context->classes.undefinedObjectClass;
        case ImmediateObjectTag_SmallInteger: return context->classes.smallIntegerClass;
        case ImmediateObjectTag_Character: return context->classes.characterClass;
        case ImmediateObjectTag_SmallFloat: return context->classes.smallFloatClass;
        }
    }

    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t *)receiver;
    return header->behavior;
}

beacon_oop_t beacon_runMethodWithArguments(beacon_context_t *context, beacon_CompiledCode_t *method, beacon_oop_t receiver, beacon_oop_t selector, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)selector;
    if(method->nativeImplementation)
        return method->nativeImplementation->nativeFunction(context, receiver, argumentCount, arguments);
    else if(method->bytecodeImplementation)
        return beacon_interpretBytecodeMethod(context, method, receiver, selector, 0, argumentCount, arguments);

    beacon_exception_error(context, "Cannot evaluate a method without any kind of implementation.");
    return 0;
}

beacon_oop_t beacon_runBlockClosureWithArguments(beacon_context_t *context, beacon_CompiledCode_t *blockClosureCode, beacon_oop_t captures, size_t argumentCount, beacon_oop_t *arguments)
{
    if(blockClosureCode->nativeImplementation)
        return blockClosureCode->nativeImplementation->nativeFunction(context, captures, argumentCount, arguments);
    else if(blockClosureCode->bytecodeImplementation)
        return beacon_interpretBytecodeMethod(context, blockClosureCode, 0, 0, captures, argumentCount, arguments);

    beacon_exception_error(context, "Cannot evaluate a block closure without code.");
    return 0;
}

beacon_oop_t beacon_performWithArgumentsInSuperclass(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, size_t argumentCount, beacon_oop_t *arguments, beacon_oop_t startingSuperclass)
{
    beacon_Behavior_t *behavior = (beacon_Behavior_t*)startingSuperclass;
    while(behavior)
    {
        if(behavior->methodDict)
        {
            beacon_CompiledCode_t *method = (beacon_CompiledCode_t *)beacon_MethodDictionary_atOrNil(context, behavior->methodDict, (beacon_Symbol_t*)selector);
            if(method)
                return beacon_runMethodWithArguments(context, method, receiver, selector, argumentCount, arguments);
        }

        behavior = behavior->superclass;
    }

    if(selector == context->roots.doesNotUnderstandSelector)
    {
        BeaconAssert(context, argumentCount == 1);
        beacon_MessageNotUnderstood_t *exception = beacon_allocateObjectWithBehavior(context->heap, context->classes.messageNotUnderstoodClass, sizeof(beacon_MessageNotUnderstood_t), BeaconObjectKindPointers);
        exception->message = (beacon_Message_t*)arguments[0];
        exception->receiver = receiver;
        char errorBuffer[256];
        snprintf(errorBuffer, sizeof(errorBuffer), "Message not understood. Selector #%.*s",
            ((beacon_Symbol_t*)exception->message->selector)->super.super.super.super.super.header.slotCount,
            ((beacon_Symbol_t*)exception->message->selector)->data);
        exception->super.super.messageText = beacon_importCString(context, errorBuffer);
        beacon_exception_signal(context, &exception->super.super);
        return 0;
    }

    beacon_Array_t *argumentsArray = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + (sizeof(beacon_oop_t)*argumentCount), BeaconObjectKindPointers);
    for(size_t i = 0; i < argumentCount; ++i)
        argumentsArray->elements[i] = arguments[i];

    beacon_Message_t *message = beacon_allocateObjectWithBehavior(context->heap, context->classes.messageClass, sizeof(beacon_Message_t), BeaconObjectKindPointers);
    message->selector = selector;
    message->arguments = (beacon_oop_t) argumentsArray;

    beacon_oop_t dnuArguments[] = {
        (beacon_oop_t)message
    };
    
    return beacon_performWithArguments(context, receiver, context->roots.doesNotUnderstandSelector, 1, dnuArguments);

}

beacon_oop_t beacon_performWithArguments(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, size_t argumentCount, beacon_oop_t *arguments)
{
    return beacon_performWithArgumentsInSuperclass(context, receiver, selector, argumentCount, arguments, (beacon_oop_t)beacon_getClass(context, receiver));
}

beacon_oop_t beacon_perform(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector)
{
    return beacon_performWithArguments(context, receiver, selector, 0, NULL);
}

beacon_oop_t beacon_performWith(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, beacon_oop_t firstArgument)
{
    beacon_oop_t arguments[] = {
        firstArgument
    };
    return beacon_performWithArguments(context, receiver, selector, 1, arguments);
}

beacon_oop_t beacon_performWithWith(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, beacon_oop_t firstArgument, beacon_oop_t secondArgument)
{
    beacon_oop_t arguments[] = {
        firstArgument,
        secondArgument
    };
    return beacon_performWithArguments(context, receiver, selector, 2, arguments);
}

double beacon_decodeNumberAsDouble(beacon_context_t *context, beacon_oop_t encodedNumber)
{
    // TODO: Support large integer and boxed double.
    return beacon_decodeSmallNumber(encodedNumber);
}

beacon_oop_t beacon_encodeDoubleAsNumber(beacon_context_t *context, double value)
{
    // TODO: Support large integer and boxed double.
    return beacon_encodeSmallFloat(value);
}

void beacon_addPrimitiveToClass(beacon_context_t *context, beacon_Behavior_t *behavior, const char *selector, size_t argumentCount, beacon_NativeCodeFunction_t primitive)
{
    beacon_Symbol_t *selectorSymbol = beacon_internCString(context, selector);
    beacon_NativeCode_t *nativeCode = beacon_allocateObjectWithBehavior(context->heap, context->classes.nativeCodeClass, sizeof(beacon_NativeCode_t), BeaconObjectKindBytes);
    nativeCode->nativeFunction = primitive;

    beacon_CompiledMethod_t *compiledMethod = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledMethodClass, sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers);
    compiledMethod->super.argumentCount = beacon_encodeSmallInteger(argumentCount);
    compiledMethod->super.nativeImplementation = nativeCode;
    if(!behavior->methodDict)
        behavior->methodDict = beacon_MethodDictionary_new(context);
    beacon_MethodDictionary_atPut(context, behavior->methodDict, selectorSymbol, (beacon_oop_t)compiledMethod);
}

static beacon_oop_t beacon_ProtoObjectPrimitive_getClass(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)argumentCount;
    (void)arguments;
    return (beacon_oop_t)beacon_getClass(context, receiver);
}

static beacon_oop_t beacon_ProtoObjectPrimitive_getIdentityHash(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)argumentCount;
    (void)arguments;
    return beacon_encodeSmallInteger(beacon_computeIdentityHash(receiver));
}

static beacon_oop_t beacon_ProtoObjectPrimitive_identityEquals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    if(receiver == arguments[0])
        return context->roots.trueValue;
    else
        return context->roots.falseValue;
}

static beacon_oop_t beacon_ProtoObjectPrimitive_identityNotEquals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    if(receiver != arguments[0])
        return context->roots.trueValue;
    else
        return context->roots.falseValue;
}

static beacon_oop_t beacon_ObjectPrimitive_yourself(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)receiver;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return receiver;
}

static beacon_oop_t beacon_ObjectPrimitive_basicSize(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    if(beacon_isImmediate(receiver))
        return beacon_encodeSmallInteger(0);

    beacon_Behavior_t *class = beacon_getClass(context, receiver);
    intptr_t fixedFieldCount = beacon_decodeSmallInteger(class->instSize);
    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t*)receiver;
    intptr_t variableFieldCount = header->slotCount - fixedFieldCount;
    return beacon_encodeSmallInteger(variableFieldCount);
}

static beacon_oop_t beacon_ObjectPrimitive_basicAt(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);

    beacon_Behavior_t *class = beacon_getClass(context, receiver);
    intptr_t fixedFieldCount = beacon_decodeSmallInteger(class->instSize);

    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t*)receiver;
    intptr_t variableFieldCount = header->slotCount - fixedFieldCount;

    // TODO: Use correct kind of exception here.
    BeaconAssert(context, 1 <= index && index <= variableFieldCount);
    if(header->objectKind == BeaconObjectKindBytes)
    {
        uint8_t *data = (uint8_t *)(header + 1);
        uint8_t value = data[index - 1];
        return beacon_encodeSmallInteger(value);
    }
    else
    {
        beacon_oop_t *data = (beacon_oop_t *)(header + 1);
        beacon_oop_t value = data[index - 1];
        return value;
    }
}

static beacon_oop_t beacon_ObjectPrimitive_basicAtPut(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);
    beacon_oop_t value = arguments[1];

    beacon_Behavior_t *class = beacon_getClass(context, receiver);
    intptr_t fixedFieldCount = beacon_decodeSmallInteger(class->instSize);

    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t*)receiver;
    intptr_t variableFieldCount = header->slotCount - fixedFieldCount;

    // TODO: Use correct kind of exception here.
    BeaconAssert(context, 1 <= index && index <= variableFieldCount);
    if(header->objectKind == BeaconObjectKindBytes)
    {
        BeaconAssert(context, beacon_isImmediate(value));
        uint8_t *data = (uint8_t *)(header + 1);
        data[index - 1] = beacon_decodeSmallInteger(value);
        return value;
    }
    else
    {
        beacon_oop_t *data = (beacon_oop_t *)(header + 1);
        data[index - 1] = value;
        return value;
    }
}
static beacon_oop_t beacon_ObjectPrimitive_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return (beacon_oop_t)beacon_importCString(context, "an Object");
}

static beacon_oop_t beacon_True_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return (beacon_oop_t)beacon_importCString(context, "true");
}

static beacon_oop_t beacon_False_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return (beacon_oop_t)beacon_importCString(context, "false");
}

static beacon_oop_t beacon_UndefinedObject_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return (beacon_oop_t)beacon_importCString(context, "nil");
}

static beacon_oop_t beacon_Class_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);
    (void)arguments;
    beacon_Class_t *clazz = (beacon_Class_t *)receiver;
    if(clazz->name)
        return (beacon_oop_t)beacon_importStringWithSize(context, clazz->name->super.super.super.super.super.header.slotCount, (const char *)clazz->name->data);
    return (beacon_oop_t)beacon_importCString(context, "A Class");
}

static beacon_oop_t beacon_String_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return receiver;
}

static beacon_oop_t beacon_SmallFloat_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%f", beacon_decodeSmallNumber(receiver));
    return (beacon_oop_t)beacon_importCString(context, buffer);
}

static beacon_oop_t beacon_SmallFloat_asFloat(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return receiver;
}

static beacon_oop_t beacon_SmallFloat_asInteger(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);

    return beacon_encodeSmallInteger(beacon_decodeSmallFloat(receiver));
}

static beacon_oop_t beacon_SmallFloat_negated(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallFloat(-beacon_decodeSmallNumber(receiver));
}

static beacon_oop_t beacon_SmallFloat_floor(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallFloat(floor(beacon_decodeSmallNumber(receiver)));
}

static beacon_oop_t beacon_SmallFloat_ceiling(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallFloat(ceil(beacon_decodeSmallNumber(receiver)));
}

static beacon_oop_t beacon_SmallFloat_plus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallFloat(beacon_decodeSmallNumber(receiver) + beacon_decodeSmallNumber(arguments[0]));
}

static beacon_oop_t beacon_SmallFloat_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallFloat(beacon_decodeSmallNumber(receiver) - beacon_decodeSmallNumber(arguments[0]));
}

static beacon_oop_t beacon_SmallFloat_times(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallFloat(beacon_decodeSmallNumber(receiver) * beacon_decodeSmallNumber(arguments[0]));
}

static beacon_oop_t beacon_SmallFloat_division(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallFloat(beacon_decodeSmallNumber(receiver) / beacon_decodeSmallNumber(arguments[0]));
}

static beacon_oop_t beacon_SmallFloat_sqrt(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallFloat(sqrt(beacon_decodeSmallNumber(receiver)));
}

static beacon_oop_t beacon_SmallFloat_equals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    return (beacon_decodeSmallNumber(receiver) == beacon_decodeSmallNumber(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallFloat_notEquals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    return (beacon_decodeSmallNumber(receiver) != beacon_decodeSmallNumber(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallFloat_lessThan(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    return (beacon_decodeSmallNumber(receiver) < beacon_decodeSmallNumber(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallFloat_lessOrEquals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    return (beacon_decodeSmallNumber(receiver) <= beacon_decodeSmallNumber(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallFloat_greaterThan(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    return (beacon_decodeSmallNumber(receiver) > beacon_decodeSmallNumber(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallFloat_greaterOrEquals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    return (beacon_decodeSmallNumber(receiver) >= beacon_decodeSmallNumber(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_Character_asInteger(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallInteger(beacon_decodeCharacter(receiver));
}

static beacon_oop_t beacon_SmallInteger_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%lld", (long long int)beacon_decodeSmallInteger(receiver));
    return (beacon_oop_t)beacon_importCString(context, buffer);
}

static beacon_oop_t beacon_SmallInteger_asFloat(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallFloat(beacon_decodeSmallInteger(receiver));
}

static beacon_oop_t beacon_SmallInteger_asInteger(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return receiver;
}

static beacon_oop_t beacon_SmallInteger_asCharacter(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeCharacter(beacon_decodeSmallInteger(receiver));
}

static beacon_oop_t beacon_SmallInteger_negated(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallInteger(-beacon_decodeSmallInteger(receiver));
}

static beacon_oop_t beacon_SmallInteger_bitInvert(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallInteger(-1 - beacon_decodeSmallInteger(receiver));
}

static beacon_oop_t beacon_SmallInteger_plus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_plus(context, receiver, argumentCount, arguments);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) + beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_minus(context, receiver, argumentCount, arguments);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) - beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_times(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_times(context, receiver, argumentCount, arguments);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) * beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_integerDivision(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) / beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_integerModulo(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) % beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_bitAnd(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) & beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_bitOr(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) | beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_bitXor(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) ^ beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_shiftLeft(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) << beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_shiftRight(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) >> beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_equals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_equals(context, receiver, argumentCount, arguments);
    return (beacon_decodeSmallInteger(receiver) == beacon_decodeSmallInteger(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallInteger_notEquals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_notEquals(context, receiver, argumentCount, arguments);
    return (beacon_decodeSmallInteger(receiver) != beacon_decodeSmallInteger(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallInteger_lessThan(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_lessThan(context, receiver, argumentCount, arguments);
    return (beacon_decodeSmallInteger(receiver) < beacon_decodeSmallInteger(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallInteger_lessOrEquals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_lessOrEquals(context, receiver, argumentCount, arguments);
    return (beacon_decodeSmallInteger(receiver) <= beacon_decodeSmallInteger(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallInteger_greaterThan(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_greaterOrEquals(context, receiver, argumentCount, arguments);
    return (beacon_decodeSmallInteger(receiver) > beacon_decodeSmallInteger(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_SmallInteger_greaterOrEquals(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    if(beacon_isSmallFloat(arguments[0]))
        return beacon_SmallFloat_greaterOrEquals(context, receiver, argumentCount, arguments);
    return (beacon_decodeSmallInteger(receiver) >= beacon_decodeSmallInteger(arguments[0])) ?
        context->roots.trueValue : context->roots.falseValue;
}

static beacon_oop_t beacon_Behavior_basicNew(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    beacon_Behavior_t *behavior = (beacon_Behavior_t *)receiver;
    size_t instanceSize = sizeof(beacon_ObjectHeader_t) + beacon_decodeSmallInteger(behavior->instSize);
    beacon_ObjectKind_t objectKind = beacon_decodeSmallInteger(behavior->objectKind);

    beacon_oop_t instance = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, behavior, instanceSize, objectKind);
    return instance;
}

static beacon_oop_t beacon_Behavior_basicNewWithSize(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    intptr_t extraSlotCount = beacon_decodeSmallInteger(arguments[0]);

    beacon_Behavior_t *behavior = (beacon_Behavior_t *)receiver;
    size_t instanceSize = sizeof(beacon_ObjectHeader_t) + beacon_decodeSmallInteger(behavior->instSize);
    beacon_ObjectKind_t objectKind = beacon_decodeSmallInteger(behavior->objectKind);

    intptr_t extraSlotsSize = extraSlotCount;
    if(objectKind != BeaconObjectKindBytes)
        extraSlotsSize *= sizeof(beacon_oop_t);

    beacon_oop_t instance = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, behavior, instanceSize + extraSlotsSize, objectKind);
    return instance;
}

static beacon_oop_t beacon_Stdio_stdin(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return context->roots.stdinStream;
}

static beacon_oop_t beacon_Stdio_stdout(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return context->roots.stdoutStream;
}

static beacon_oop_t beacon_Stdio_stderr(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return context->roots.stderrStream;
}

static beacon_oop_t beacon_AbstractBinaryFileStream_nextPut(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    beacon_AbstractBinaryFileStream_t *stream = (beacon_AbstractBinaryFileStream_t *)receiver;
    int fd = beacon_decodeSmallInteger(stream->handle);
    uint8_t value = beacon_decodeSmallInteger(arguments[0]);
    
    ssize_t writtenCount = write(fd, &value, 1);
    BeaconAssert(context, writtenCount == 1);
    return receiver;
}

static beacon_oop_t beacon_AbstractBinaryFileStream_nextPutAll(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 1);
    
    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t *)arguments[0];
    ssize_t objectSize = header->slotCount;
    BeaconAssert(context, header->objectKind == BeaconObjectKindBytes);
    uint8_t *objectData = (uint8_t *)(header + 1);

    beacon_AbstractBinaryFileStream_t *stream = (beacon_AbstractBinaryFileStream_t *)receiver;
    int fd = beacon_decodeSmallInteger(stream->handle);
    
    ssize_t writtenCount = write(fd, objectData, objectSize);
    BeaconAssert(context, writtenCount == objectSize);
    return receiver;
}

static beacon_oop_t beacon_String_concatenate(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)receiver;
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 1);

    beacon_ObjectHeader_t *receiverHeader = (beacon_ObjectHeader_t *)receiver;
    ssize_t receiverSize = receiverHeader->slotCount;
    if(receiverSize == 0)
        return arguments[0];

    beacon_ObjectHeader_t *header = (beacon_ObjectHeader_t *)arguments[0];
    ssize_t objectSize = header->slotCount;
    if(objectSize == 0)
        return receiver;

    BeaconAssert(context, receiverHeader->objectKind == BeaconObjectKindBytes);
    BeaconAssert(context, header->objectKind == BeaconObjectKindBytes);
    uint8_t *receiverData = (uint8_t *)(receiverHeader + 1);
    uint8_t *objectData = (uint8_t *)(header + 1);

    beacon_String_t *concatResult = beacon_allocateObjectWithBehavior(context->heap, context->classes.stringClass, sizeof(beacon_String_t) + receiverSize + objectSize, BeaconObjectKindBytes);
    memcpy(concatResult->data, receiverData, receiverSize);
    memcpy(concatResult->data + receiverSize, objectData, objectSize);

    return (beacon_oop_t)concatResult;
}

static beacon_oop_t beacon_String_fileIn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    beacon_String_t *receiverString = (beacon_String_t *)receiver;
    size_t stringSize = receiverString->super.super.super.super.super.header.slotCount;

    char *stringBuffer = calloc(1, stringSize + 1);
    memcpy(stringBuffer, receiverString->data, stringSize);

    beacon_SourceCode_t *sourceCode = beacon_makeSourceCodeFromFileNamed(context, stringBuffer);
    free(stringBuffer);

    return beacon_evaluateSourceCode(context, sourceCode);
}

static beacon_oop_t beacon_String_asString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)argumentCount;
    (void)arguments;
    return receiver;
}

static beacon_oop_t beacon_String_asSymbol(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)argumentCount;
    (void)arguments;
    return (beacon_oop_t)beacon_internString(context, (beacon_String_t*)receiver);
}

static beacon_oop_t beacon_Symbol_asString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)argumentCount;
    (void)arguments;
    beacon_Symbol_t *symbol = (beacon_Symbol_t *)receiver;
    size_t symbolSize = symbol->super.super.super.super.super.header.slotCount;
    beacon_String_t *string = beacon_allocateObjectWithBehavior(context->heap, context->classes.stringClass, sizeof(beacon_String_t) + symbolSize, BeaconObjectKindBytes);
    memcpy(string->data, symbol->data, symbolSize);
    return (beacon_oop_t)string;
}

static beacon_oop_t beacon_Symbol_asSymbol(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)argumentCount;
    (void)arguments;
    return receiver;
}

static beacon_oop_t beacon_Object_value(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)argumentCount;
    (void)arguments;
    return receiver;
}

static beacon_oop_t beacon_BlockClosure_value(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_BlockClosure_t* blockClosure = (beacon_BlockClosure_t*)receiver;
    intptr_t expectedArgumentCount = beacon_decodeSmallInteger(blockClosure->code->super.argumentCount);
    BeaconAssert(context, (intptr_t)argumentCount == expectedArgumentCount);

    return beacon_runBlockClosureWithArguments(context, &blockClosure->code->super, blockClosure->captures, argumentCount, arguments);
}

static beacon_oop_t beacon_BlockClosure_ensure(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_BlockClosure_t* blockClosureReceiver = (beacon_BlockClosure_t*)receiver;
    beacon_BlockClosure_t* ensureBlock = (beacon_BlockClosure_t*)arguments[0];

    beacon_StackFrameRecord_t ensureRecord = {
        .context = context,
        .kind = StackFrameEnsure,
        .ensure = {
            .ensureReceiver = (beacon_oop_t)blockClosureReceiver,
            .ensureBlock = (beacon_oop_t)ensureBlock,
            .resultValue = 0,
        }
    };
    beacon_pushStackFrameRecord(&ensureRecord);

    ensureRecord.ensure.resultValue = beacon_runBlockClosureWithArguments(context, &blockClosureReceiver->code->super, blockClosureReceiver->captures, 0, NULL);
    beacon_runBlockClosureWithArguments(context, &ensureBlock->code->super, ensureBlock->captures, 0, NULL);

    beacon_popStackFrameRecord(&ensureRecord);
    return ensureRecord.ensure.resultValue;
}

static beacon_oop_t beacon_BlockClosure_onDo(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);

    beacon_BlockClosure_t* blockClosureReceiver = (beacon_BlockClosure_t*)receiver;
    beacon_oop_t onFilter = arguments[0];
    beacon_BlockClosure_t *doBlock = (beacon_BlockClosure_t*)arguments[1];

    beacon_StackFrameRecord_t onDoRecord = {
        .context = context,
        .kind = StackFrameOnDo,
        .onDo = {
            .onReceiver = (beacon_oop_t)blockClosureReceiver,
            .onFilter = onFilter,
            .doBlock = (beacon_oop_t)doBlock,
            .resultValue = 0,
            .exception = 0,
        }
    };

    beacon_pushStackFrameRecord(&onDoRecord);

    if(_setjmp(onDoRecord.onDo.handlerJumpBuffer))
    {
        onDoRecord.onDo.resultValue = beacon_runBlockClosureWithArguments(context, &doBlock->code->super, doBlock->captures, 0, NULL); 
    }
    else
    {
        onDoRecord.onDo.resultValue = beacon_runBlockClosureWithArguments(context, &blockClosureReceiver->code->super, blockClosureReceiver->captures, 0, NULL);
    }

    beacon_popStackFrameRecord(&onDoRecord);
    return onDoRecord.onDo.resultValue;

}

beacon_oop_t beacon_boxExternalAddress(beacon_context_t *context, void *pointer)
{
    beacon_ExternalAddress_t *externalAddress = beacon_allocateObjectWithBehavior(context->heap, context->classes.externalAddressClass, sizeof(beacon_ExternalAddress_t), BeaconObjectKindBytes);
    externalAddress->address = pointer;
    return (beacon_oop_t)externalAddress;
}

void *beacon_unboxExternalAddress(beacon_context_t *context, beacon_oop_t box)
{
    if(!box)
        return NULL;
    BeaconAssert(context, beacon_getClass(context, box) == context->classes.externalAddressClass);
    return ((beacon_ExternalAddress_t*)box)->address;
}

void beacon_context_registerObjectBasicPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.protoObjectClass, "class", 0, beacon_ProtoObjectPrimitive_getClass);
    beacon_addPrimitiveToClass(context, context->classes.protoObjectClass, "identityHash", 0, beacon_ProtoObjectPrimitive_getIdentityHash);
    beacon_addPrimitiveToClass(context, context->classes.protoObjectClass, "==", 1, beacon_ProtoObjectPrimitive_identityEquals);
    beacon_addPrimitiveToClass(context, context->classes.protoObjectClass, "~~", 1, beacon_ProtoObjectPrimitive_identityNotEquals);

    beacon_addPrimitiveToClass(context, context->classes.objectClass, "yourself", 0, beacon_ObjectPrimitive_yourself);
    beacon_addPrimitiveToClass(context, context->classes.objectClass, "basicSize", 0, beacon_ObjectPrimitive_basicSize);
    beacon_addPrimitiveToClass(context, context->classes.objectClass, "basicAt:", 0, beacon_ObjectPrimitive_basicAt);
    beacon_addPrimitiveToClass(context, context->classes.objectClass, "basicAt:put:", 0, beacon_ObjectPrimitive_basicAtPut);

    beacon_addPrimitiveToClass(context, context->classes.objectClass, "printString", 0, beacon_ObjectPrimitive_printString);
    beacon_addPrimitiveToClass(context, context->classes.trueClass, "printString", 0, beacon_True_printString);
    beacon_addPrimitiveToClass(context, context->classes.falseClass, "printString", 0, beacon_False_printString);
    beacon_addPrimitiveToClass(context, context->classes.undefinedObjectClass, "printString", 0, beacon_UndefinedObject_printString);
    beacon_addPrimitiveToClass(context, context->classes.classClass, "printString", 0, beacon_Class_printString);
    beacon_addPrimitiveToClass(context, context->classes.stringClass, "printString", 0, beacon_String_printString);

    beacon_addPrimitiveToClass(context, context->classes.characterClass, "asInteger", 0, beacon_Character_asInteger);

    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "printString", 0, beacon_SmallInteger_printString);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "asFloat", 0, beacon_SmallInteger_asFloat);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "asInteger", 0, beacon_SmallInteger_asInteger);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "asCharacter", 0, beacon_SmallInteger_asCharacter);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "negated", 0, beacon_SmallInteger_negated);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "+", 1, beacon_SmallInteger_plus);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "-", 1, beacon_SmallInteger_minus);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "*", 1, beacon_SmallInteger_times);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "//", 1, beacon_SmallInteger_integerDivision);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "\\", 1, beacon_SmallInteger_integerModulo);

    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "bitInvert", 0, beacon_SmallInteger_bitInvert);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "|", 1, beacon_SmallInteger_bitOr);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "bitOr:", 1, beacon_SmallInteger_bitOr);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "&", 1, beacon_SmallInteger_bitAnd);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "bitAnd:", 1, beacon_SmallInteger_bitAnd);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "^", 1, beacon_SmallInteger_bitXor);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "bitXor:", 1, beacon_SmallInteger_bitXor);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "<<", 1, beacon_SmallInteger_shiftLeft);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, ">>", 1, beacon_SmallInteger_shiftRight);

    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "=", 1, beacon_SmallInteger_equals);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "~=", 1, beacon_SmallInteger_notEquals);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "<", 1, beacon_SmallInteger_lessThan);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "<=", 1, beacon_SmallInteger_lessOrEquals);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, ">", 1, beacon_SmallInteger_greaterThan);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, ">=", 1, beacon_SmallInteger_greaterOrEquals);

    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "printString", 0, beacon_SmallFloat_printString);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "asFloat", 0, beacon_SmallFloat_asFloat);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "asInteger", 0, beacon_SmallFloat_asInteger);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "floor", 0, beacon_SmallFloat_floor);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "ceiling", 0, beacon_SmallFloat_ceiling);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "negated", 0, beacon_SmallFloat_negated);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "sqrt", 0, beacon_SmallFloat_sqrt);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "+", 1, beacon_SmallFloat_plus);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "-", 1, beacon_SmallFloat_minus);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "*", 1, beacon_SmallFloat_times);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "/", 1, beacon_SmallFloat_division);

    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "=", 1, beacon_SmallFloat_equals);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "~=", 1, beacon_SmallFloat_notEquals);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "<", 1, beacon_SmallFloat_lessThan);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, "<=", 1, beacon_SmallFloat_lessOrEquals);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, ">", 1, beacon_SmallFloat_greaterThan);
    beacon_addPrimitiveToClass(context, context->classes.smallFloatClass, ">=", 1, beacon_SmallFloat_greaterOrEquals);

    beacon_addPrimitiveToClass(context, context->classes.behaviorClass, "basicNew", 0, beacon_Behavior_basicNew);
    beacon_addPrimitiveToClass(context, context->classes.behaviorClass, "basicNew:", 1, beacon_Behavior_basicNewWithSize);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.stdioClass), "stdin", 0, beacon_Stdio_stdin);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.stdioClass), "stdout", 0, beacon_Stdio_stdout);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.stdioClass), "stderr", 0, beacon_Stdio_stderr);

    beacon_addPrimitiveToClass(context, context->classes.abstractBinaryFileStreamClass, "nextPut:", 1, beacon_AbstractBinaryFileStream_nextPut);
    beacon_addPrimitiveToClass(context, context->classes.abstractBinaryFileStreamClass, "nextPutAll:", 1, beacon_AbstractBinaryFileStream_nextPutAll);

    beacon_addPrimitiveToClass(context, context->classes.stringClass, ",", 1, beacon_String_concatenate);
    beacon_addPrimitiveToClass(context, context->classes.stringClass, "fileIn", 1, beacon_String_fileIn);
    beacon_addPrimitiveToClass(context, context->classes.stringClass, "asString", 1, beacon_String_asString);
    beacon_addPrimitiveToClass(context, context->classes.stringClass, "asSymbol", 1, beacon_String_asSymbol);

    beacon_addPrimitiveToClass(context, context->classes.symbolClass, "asString", 1, beacon_Symbol_asString);
    beacon_addPrimitiveToClass(context, context->classes.symbolClass, "asSymbol", 1, beacon_Symbol_asSymbol);

    beacon_addPrimitiveToClass(context, context->classes.objectClass, "value", 0, beacon_Object_value);

    beacon_addPrimitiveToClass(context, context->classes.blockClosureClass, "value", 0, beacon_BlockClosure_value);
    beacon_addPrimitiveToClass(context, context->classes.blockClosureClass, "value:", 1, beacon_BlockClosure_value);
    beacon_addPrimitiveToClass(context, context->classes.blockClosureClass, "value:value:", 2, beacon_BlockClosure_value);
    beacon_addPrimitiveToClass(context, context->classes.blockClosureClass, "value:value:value:", 3, beacon_BlockClosure_value);
    beacon_addPrimitiveToClass(context, context->classes.blockClosureClass, "value:value:value:value:", 4, beacon_BlockClosure_value);

    beacon_addPrimitiveToClass(context, context->classes.blockClosureClass, "ensure:", 1, beacon_BlockClosure_ensure);
    beacon_addPrimitiveToClass(context, context->classes.blockClosureClass, "on:do:", 2, beacon_BlockClosure_onDo);

}

