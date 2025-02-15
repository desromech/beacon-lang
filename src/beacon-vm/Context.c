#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Dictionary.h"
#include "beacon-lang/Exceptions.h"
#include "beacon-lang/Bytecode.h"
#include "beacon-lang/ArrayList.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

void beacon_context_registerObjectBasicPrimitives(beacon_context_t *context);
void beacon_context_registerParseTreeCompilationPrimitives(beacon_context_t *context);

static beacon_Behavior_t *beacon_context_createClassAndMetaclass(beacon_context_t *context, beacon_Behavior_t *superclassBehavior, const char *name, size_t instanceSize, beacon_ObjectKind_t objectKind, ...)
{
    BeaconAssert(context, instanceSize >= sizeof(beacon_ObjectHeader_t));

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
            continue;
        }

        beacon_Symbol_t *varNameSymbol = beacon_internCString(context, nextVarName);
        if(parsingMetaVariable)
            beacon_ArrayList_add(context, metaClassVarNames, (beacon_oop_t)varNameSymbol);
        else
            beacon_ArrayList_add(context, classVarNames, (beacon_oop_t)varNameSymbol);
        
        nextVarName = va_arg(varNames, const char *);
    }

    va_end(varNames);

    beacon_Metaclass_t *metaclass = beacon_allocateObjectWithBehavior(context->heap, context->classes.metaclassClass, sizeof(beacon_Metaclass_t), BeaconObjectKindPointers);
    metaclass->super.super.instSize = beacon_encodeSmallInteger(sizeof(beacon_Class_t) - sizeof(beacon_ObjectHeader_t));
    metaclass->super.super.objectKind = beacon_encodeSmallInteger(BeaconObjectKindPointers);
    metaclass->super.super.instanceVariableNames = (beacon_oop_t)beacon_ArrayList_asArray(context, metaClassVarNames);

    beacon_Class_t *clazz = beacon_allocateObjectWithBehavior(context->heap, (beacon_Behavior_t*)metaclass, sizeof(beacon_Class_t), BeaconObjectKindPointers);
    metaclass->thisClass = clazz;
    if(name)
        clazz->name = beacon_internCString(context, name);
    clazz->super.super.instSize = beacon_encodeSmallInteger(instanceSize - sizeof(beacon_ObjectHeader_t));
    clazz->super.super.objectKind = beacon_encodeSmallInteger(objectKind);
    clazz->super.super.instanceVariableNames = (beacon_oop_t)beacon_ArrayList_asArray(context, classVarNames);

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
    context->classes.protoObjectClass = beacon_context_createClassAndMetaclass(context, NULL, "ProtoObject", sizeof(beacon_ProtoObject_t), BeaconObjectKindPointers, NULL);
    context->classes.objectClass = beacon_context_createClassAndMetaclass(context, context->classes.protoObjectClass, "Object", sizeof(beacon_Object_t), BeaconObjectKindPointers, NULL);
    context->classes.behaviorClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Behavior", sizeof(beacon_Behavior_t), BeaconObjectKindPointers,
        "superclass", "methodDict", "instSize", "objectKind", "instanceVariableNames", NULL);
    context->classes.classDescriptionClass = beacon_context_createClassAndMetaclass(context, context->classes.behaviorClass, "ClassDescription", sizeof(beacon_ClassDescription_t), BeaconObjectKindPointers,
        "protocols", NULL);
    context->classes.metaclassClass = beacon_context_createClassAndMetaclass(context, context->classes.classDescriptionClass, "Metaclass", sizeof(beacon_Metaclass_t), BeaconObjectKindPointers,
        "thisClass", NULL);
    
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
    context->classes.dictionaryClass = beacon_context_createClassAndMetaclass(context, context->classes.hashedCollectionClass, "Dictionary", sizeof(beacon_Dictionary_t), BeaconObjectKindPointers, NULL);
    context->classes.methodDictionaryClass = beacon_context_createClassAndMetaclass(context, context->classes.methodDictionaryClass, "MethodDictionary", sizeof(beacon_MethodDictionary_t), BeaconObjectKindPointers, NULL);
    context->classes.sequenceableCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.collectionClass, "SequenceableCollection", sizeof(beacon_SequenceableCollection_t), BeaconObjectKindPointers, NULL);
    context->classes.arrayListClass = beacon_context_createClassAndMetaclass(context, context->classes.sequenceableCollectionClass, "ArrayList", sizeof(beacon_ArrayList_t), BeaconObjectKindPointers,
        "array", "size", "capacity", NULL);
    context->classes.byteArrayListClass = beacon_context_createClassAndMetaclass(context, context->classes.sequenceableCollectionClass, "ByteArrayList", sizeof(beacon_ByteArrayList_t), BeaconObjectKindPointers,
        "array", "size", "capacity", NULL);
    context->classes.arrayedCollectionClass = beacon_context_createClassAndMetaclass(context, context->classes.sequenceableCollectionClass, "ArrayedCollection", sizeof(beacon_ArrayedCollection_t), BeaconObjectKindPointers, NULL);
    context->classes.arrayClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "Array", sizeof(beacon_Array_t), BeaconObjectKindPointers, NULL);
    context->classes.byteArrayClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "ByteArray", sizeof(beacon_ByteArray_t), BeaconObjectKindBytes, NULL);
    context->classes.stringClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "String", sizeof(beacon_String_t), BeaconObjectKindBytes, NULL);
    context->classes.symbolClass = beacon_context_createClassAndMetaclass(context, context->classes.arrayedCollectionClass, "Symbol", sizeof(beacon_Symbol_t), BeaconObjectKindBytes, NULL);

    context->classes.booleanClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Boolean", sizeof(beacon_Boolean_t), BeaconObjectKindPointers, NULL);
    context->classes.trueClass = beacon_context_createClassAndMetaclass(context, context->classes.booleanClass, "True", sizeof(beacon_True_t), BeaconObjectKindPointers, NULL);
    context->classes.falseClass = beacon_context_createClassAndMetaclass(context, context->classes.booleanClass, "False", sizeof(beacon_False_t), BeaconObjectKindPointers, NULL);
    context->classes.undefinedObjectClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "UndefinedObject", sizeof(beacon_UndefinedObject_t), BeaconObjectKindImmediate, NULL);

    context->classes.magnitudeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Magnitude", sizeof(beacon_Magnitude_t), BeaconObjectKindPointers, NULL);
    context->classes.numberClass = beacon_context_createClassAndMetaclass(context, context->classes.magnitudeClass, "Number", sizeof(beacon_Number_t), BeaconObjectKindPointers, NULL);
    context->classes.integerClass = beacon_context_createClassAndMetaclass(context, context->classes.numberClass, "Integer", sizeof(beacon_Integer_t), BeaconObjectKindPointers, NULL);
    context->classes.smallIntegerClass = beacon_context_createClassAndMetaclass(context, context->classes.integerClass, "SmallInteger", sizeof(beacon_SmallInteger_t), BeaconObjectKindImmediate, NULL);
    context->classes.floatClass = beacon_context_createClassAndMetaclass(context, context->classes.numberClass, "Float", sizeof(beacon_Float_t), BeaconObjectKindImmediate, NULL);
    context->classes.smallFloatClass = beacon_context_createClassAndMetaclass(context, context->classes.floatClass, "SmallFloat", sizeof(beacon_SmallFloat_t), BeaconObjectKindImmediate, NULL);

    context->classes.characterClass = beacon_context_createClassAndMetaclass(context, context->classes.magnitudeClass, "Character", sizeof(beacon_Character_t), BeaconObjectKindImmediate, NULL);

    context->classes.nativeCodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "NativeCode", sizeof(beacon_NativeCode_t), BeaconObjectKindBytes, NULL);
    context->classes.bytecodeCodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "BytecodeCode", sizeof(beacon_BytecodeCode_t), BeaconObjectKindPointers,
        "argumentCount", "temporaryCount", "literals", "bytecodes", NULL);
    context->classes.bytecodeCodeBuilderClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "BytecodeCodeBuilder", sizeof(beacon_BytecodeCodeBuilder_t), BeaconObjectKindPointers,
        "arguments", "temporaries", "literals", "bytecodes", NULL);
    context->classes.compiledCodeClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "CompiledCode", sizeof(beacon_CompiledCode_t), BeaconObjectKindPointers,
        "argumentCount",  "nativeImplementation", "bytecodeImplementation", NULL);
    context->classes.compiledBlockClass = beacon_context_createClassAndMetaclass(context, context->classes.compiledCodeClass, "CompiledBlock", sizeof(beacon_CompiledBlock_t), BeaconObjectKindPointers,
        "captureCount", NULL);
    context->classes.compiledMethodClass = beacon_context_createClassAndMetaclass(context, context->classes.compiledCodeClass, "CompiledMethod", sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers,
        "name", NULL);
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

    context->classes.abstractBinaryFileStream = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "AbstractBinaryFileStream", sizeof(beacon_Stdio_t), BeaconObjectKindPointers,
        "handle", NULL);
    context->classes.stdioClass = beacon_context_createClassAndMetaclass(context, context->classes.objectClass, "Stdio", sizeof(beacon_Stdio_t), BeaconObjectKindPointers, NULL);
    context->classes.stdioStreamClass = beacon_context_createClassAndMetaclass(context, context->classes.abstractBinaryFileStream, "StdioStream", sizeof(beacon_Stdio_t), BeaconObjectKindPointers, NULL);
}

void beacon_context_createImportantRoots(beacon_context_t *context)
{
    context->roots.trueValue = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, context->classes.trueClass, sizeof(beacon_True_t), BeaconObjectKindPointers);
    context->roots.falseValue = (beacon_oop_t)beacon_allocateObjectWithBehavior(context->heap, context->classes.falseClass, sizeof(beacon_False_t), BeaconObjectKindPointers);
    context->roots.doesNotUnderstandSelector = (beacon_oop_t)beacon_internCString(context, "doesNotUnderstand:");
    context->roots.compileWithEnvironmentAndBytecodeBuilderSelector = (beacon_oop_t)beacon_internCString(context, "compileWithEnvironment:andBytecodeBuilder:");
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
            beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, globalClass->name, (beacon_oop_t)globalClass);
    }

    beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, beacon_internCString(context, "nil"), context->roots.nilValue);
    beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, beacon_internCString(context, "true"), context->roots.trueValue);
    beacon_MethodDictionary_atPut(context, context->roots.systemDictionary, beacon_internCString(context, "false"), context->roots.falseValue);
}

void beacon_context_registerBasicPrimitives(beacon_context_t *context)
{
    beacon_context_registerObjectBasicPrimitives(context);
    beacon_context_registerParseTreeCompilationPrimitives(context);
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
        return beacon_interpretBytecodeMethod(context, method, receiver, selector, argumentCount, arguments);

    beacon_exception_error(context, "Cannot evaluate a method without any kind of implementation.");
    return 0;
}

beacon_oop_t beacon_performWithArguments(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_Behavior_t *behavior = beacon_getClass(context, receiver);
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
        exception->super.super.messageText = beacon_importCString(context, "Message not understood.");
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


static beacon_oop_t beacon_SmallInteger_printString(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%lld", (long long int)beacon_decodeSmallInteger(receiver));
    return (beacon_oop_t)beacon_importCString(context, buffer);
}

static beacon_oop_t beacon_SmallInteger_negated(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 0);
    return beacon_encodeSmallInteger(-beacon_decodeSmallInteger(receiver));
}

static beacon_oop_t beacon_SmallInteger_plus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) + beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
    return beacon_encodeSmallInteger(beacon_decodeSmallInteger(receiver) - beacon_decodeSmallInteger(arguments[0]));
}

static beacon_oop_t beacon_SmallInteger_times(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    BeaconAssert(context, argumentCount == 1);
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
    BeaconAssert(context, header->objectKind = BeaconObjectKindBytes);
    uint8_t *objectData = (uint8_t *)(header + 1);

    beacon_AbstractBinaryFileStream_t *stream = (beacon_AbstractBinaryFileStream_t *)receiver;
    int fd = beacon_decodeSmallInteger(stream->handle);
    
    ssize_t writtenCount = write(fd, objectData, objectSize);
    BeaconAssert(context, writtenCount == objectSize);
    return receiver;
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

    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "printString", 0, beacon_SmallInteger_printString);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "negated", 0, beacon_SmallInteger_negated);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "+", 1, beacon_SmallInteger_plus);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "-", 1, beacon_SmallInteger_minus);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "*", 1, beacon_SmallInteger_times);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "//", 1, beacon_SmallInteger_integerDivision);
    beacon_addPrimitiveToClass(context, context->classes.smallIntegerClass, "\\", 1, beacon_SmallInteger_integerModulo);

    beacon_addPrimitiveToClass(context, context->classes.behaviorClass, "basicNew", 0, beacon_Behavior_basicNew);
    beacon_addPrimitiveToClass(context, context->classes.behaviorClass, "basicNew:", 1, beacon_Behavior_basicNewWithSize);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.stdioClass), "stdin", 0, beacon_Stdio_stdin);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.stdioClass), "stdout", 0, beacon_Stdio_stdout);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.stdioClass), "stderr", 0, beacon_Stdio_stderr);

    beacon_addPrimitiveToClass(context, context->classes.abstractBinaryFileStream, "nextPut:", 1, beacon_AbstractBinaryFileStream_nextPut);
    beacon_addPrimitiveToClass(context, context->classes.abstractBinaryFileStream, "nextPutAll:", 1, beacon_AbstractBinaryFileStream_nextPutAll);
}
