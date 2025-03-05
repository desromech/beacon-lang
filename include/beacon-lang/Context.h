#ifndef BEACON_CONTEXT_H
#define BEACON_CONTEXT_H

#pragma once

#include "ObjectModel.h"
#include "Memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct beacon_context_s beacon_context_t;

struct beacon_context_s
{
    struct ContextClasses
    {
        beacon_Behavior_t *protoObjectClass;
        beacon_Behavior_t *objectClass;
        beacon_Behavior_t *collectionClass;
        beacon_Behavior_t *hashedCollectionClass;
        beacon_Behavior_t *associationClass;
        beacon_Behavior_t *dictionaryClass;
        beacon_Behavior_t *methodDictionaryClass;
        beacon_Behavior_t *slotClass;
        beacon_Behavior_t *behaviorClass;
        beacon_Behavior_t *classDescriptionClass;
        beacon_Behavior_t *classClass;
        beacon_Behavior_t *metaclassClass;

        beacon_Behavior_t *sequenceableCollectionClass;
        beacon_Behavior_t *arrayListClass;
        beacon_Behavior_t *byteArrayListClass;
        beacon_Behavior_t *arrayedCollectionClass;
        beacon_Behavior_t *arrayClass;
        beacon_Behavior_t *byteArrayClass;
        beacon_Behavior_t *externalAddressClass;
        beacon_Behavior_t *stringClass;
        beacon_Behavior_t *symbolClass;

        beacon_Behavior_t *booleanClass;
        beacon_Behavior_t *trueClass;
        beacon_Behavior_t *falseClass;
        beacon_Behavior_t *undefinedObjectClass;

        beacon_Behavior_t *magnitudeClass;
        beacon_Behavior_t *numberClass;
        beacon_Behavior_t *integerClass;
        beacon_Behavior_t *floatClass;
        beacon_Behavior_t *smallIntegerClass;
        beacon_Behavior_t *characterClass;
        beacon_Behavior_t *smallFloatClass;

        beacon_Behavior_t *nativeCodeClass;
        beacon_Behavior_t *bytecodeCodeClass;
        beacon_Behavior_t *bytecodeCodeBuilderClass;
        beacon_Behavior_t *compiledCodeClass;
        beacon_Behavior_t *compiledBlockClass;
        beacon_Behavior_t *compiledMethodClass;
        beacon_Behavior_t *blockClosureClass;
        beacon_Behavior_t *messageClass;

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
        beacon_Behavior_t *parseTreeAssignmentNodeClass;
        beacon_Behavior_t *parseTreeArrayNodeClass;
        beacon_Behavior_t *parseTreeByteArrayNodeClass;
        beacon_Behavior_t *parseTreeLiteralArrayNodeClass;
        beacon_Behavior_t *parseTreeArgumentDefinitionNodeClass;
        beacon_Behavior_t *parseTreeLocalVariableDefinitionNodeClass;
        beacon_Behavior_t *parseTreeBlockClosureNodeClass;
        beacon_Behavior_t *parseTreeAddMethodNodeClass;
        beacon_Behavior_t *parseTreeMethodNode;
        beacon_Behavior_t *parseTreeWorkspaceScriptNode;

        beacon_Behavior_t *abstractCompilationEnvironmentClass;
        beacon_Behavior_t *emptyCompilationEnvironmentClass;
        beacon_Behavior_t *systemCompilationEnvironmentClass;
        beacon_Behavior_t *fileCompilationEnvironmentClass;
        beacon_Behavior_t *lexicalCompilationEnvironmentClass;
        beacon_Behavior_t *blockClosureCompilationEnvironmentClass;
        beacon_Behavior_t *methodCompilationEnvironmentClass;
        beacon_Behavior_t *behaviorCompilationEnvironmentClass;

        beacon_Behavior_t *exceptionClass;
        beacon_Behavior_t *errorClass;
        beacon_Behavior_t *assertionFailureClass;
        beacon_Behavior_t *messageNotUnderstoodClass;
        beacon_Behavior_t *nonBooleanReceiverClass;
        beacon_Behavior_t *unhandledExceptionClass;
        beacon_Behavior_t *unhandledErrorClass;
        
        beacon_Behavior_t *weakTombstoneClass;

        beacon_Behavior_t *streamClass;
        beacon_Behavior_t *abstractBinaryFileStreamClass;
        beacon_Behavior_t *stdioClass;
        beacon_Behavior_t *stdioStreamClass;

        beacon_Behavior_t *pointClass;
        beacon_Behavior_t *colorClass;
        beacon_Behavior_t *rectangleClass;
        beacon_Behavior_t *formClass;
        beacon_Behavior_t *fontClass;
        beacon_Behavior_t *fontFaceClass;

        beacon_Behavior_t *formRenderingElementClass;
        beacon_Behavior_t *formSolidRectangleRenderingElementClass;
        beacon_Behavior_t *formHorizontalGradientRenderingElementClass;
        beacon_Behavior_t *formVerticalGradientRenderingElementClass;
        beacon_Behavior_t *formTextRenderingElementClass;

        beacon_Behavior_t *windowClass;
        beacon_Behavior_t *windowEventClass;
        beacon_Behavior_t *windowExposeEventClass;
        beacon_Behavior_t *windowMouseButtonEventClass;
        beacon_Behavior_t *windowMouseMotionEventClass;
        beacon_Behavior_t *windowMouseWheelEventClass;
        beacon_Behavior_t *windowKeyboardEventClass;
        beacon_Behavior_t *windowTextInputEventClass;

        beacon_Behavior_t *testAsserterClass;
        beacon_Behavior_t *testCaseClass;

        beacon_Behavior_t *abstractPrimitiveTensorClass;
        beacon_Behavior_t *abstractPrimitiveMatrixClass;
        beacon_Behavior_t *matrix2x2Class;
        beacon_Behavior_t *matrix3x3Class;
        beacon_Behavior_t *matrix4x4Class;
        beacon_Behavior_t *abstractPrimitiveVectorClass;
        beacon_Behavior_t *vector2Class;
        beacon_Behavior_t *vector3Class;
        beacon_Behavior_t *vector4Class;

        beacon_Behavior_t *complexClass;
        beacon_Behavior_t *quaternionClass;

        beacon_Behavior_t *agpuClass;
        beacon_Behavior_t *agpuSwapChainClass;
        beacon_Behavior_t *agpuWindowRendererClass;
    } classes;

    struct ContextGCRoots
    {
        beacon_InternedSymbolSet_t *internedSymbolSet;
        beacon_MethodDictionary_t *systemDictionary;
        beacon_oop_t nilValue;
        beacon_oop_t trueValue;
        beacon_oop_t falseValue;
        beacon_oop_t doesNotUnderstandSelector;
        beacon_oop_t compileWithEnvironmentAndBytecodeBuilderSelector;
        beacon_oop_t compileInlineBlockWithArgumentsWithEnvironmentAndBytecodeBuilderSelector;
        beacon_oop_t lookupSymbolRecursivelySelector;
        beacon_oop_t lookupSymbolRecursivelyWithBytecodeBuilderSelector;
        beacon_oop_t evaluateWithEnvironmentSelector;
        beacon_oop_t addCompiledMethodSelector;
        beacon_oop_t emptyArray;
        beacon_oop_t weakTombstone;

        beacon_oop_t stderrStream;
        beacon_oop_t stdoutStream;
        beacon_oop_t stdinStream;

        beacon_oop_t ifTrueSelector;
        beacon_oop_t ifFalseSelector;
        beacon_oop_t ifTrueIfFalseSelector;
        beacon_oop_t ifFalseIfTrueSelector;
        beacon_oop_t andSelector;
        beacon_oop_t orSelector;
        beacon_oop_t whileTrueSelector;
        beacon_oop_t whileFalseSelector;
        beacon_oop_t whileTrueDoSelector;
        beacon_oop_t whileFalseDoSelector;
        beacon_oop_t doWhileTrueSelector;
        beacon_oop_t doWhileFalseSelector;
        beacon_oop_t toDoSelector;

        beacon_oop_t plusSelector;
        beacon_oop_t lessOrEqualsSelector;

        beacon_MethodDictionary_t *windowHandleMap;
        struct beacon_AGPU_s *agpuCommon;
    } roots;

    beacon_MemoryHeap_t *heap;
};

beacon_context_t *beacon_context_new(void);
void beacon_context_destroy(beacon_context_t *context);

uint32_t beacon_computeStringHash(size_t stringSize, const char *string);
uint32_t beacon_computeIdentityHash(beacon_oop_t oop);

beacon_String_t *beacon_importCString(beacon_context_t *context, const char *string);
beacon_String_t *beacon_importStringWithSize(beacon_context_t *context, size_t stringSize, const char *string);
beacon_Symbol_t *beacon_internStringWithSize(beacon_context_t *context, size_t stringSize, const char *string);
beacon_Symbol_t *beacon_internCString(beacon_context_t *context, const char *string);
beacon_Symbol_t *beacon_internString(beacon_context_t *context, beacon_String_t *string);

beacon_Behavior_t *beacon_getClass(beacon_context_t *context, beacon_oop_t receiver);

beacon_oop_t beacon_performWithArgumentsInSuperclass(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, size_t argumentCount, beacon_oop_t *arguments, beacon_oop_t superclass);
beacon_oop_t beacon_performWithArguments(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, size_t argumentCount, beacon_oop_t *arguments);
beacon_oop_t beacon_perform(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector);
beacon_oop_t beacon_performWith(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, beacon_oop_t firstArgument);
beacon_oop_t beacon_performWithWith(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector, beacon_oop_t firstArgument, beacon_oop_t secondArgument);

beacon_oop_t beacon_runMethodWithArguments(beacon_context_t *context, beacon_CompiledCode_t *method, beacon_oop_t receiver, beacon_oop_t selector, size_t argumentCount, beacon_oop_t *arguments);

void beacon_addPrimitiveToClass(beacon_context_t *context, beacon_Behavior_t *behavior, const char *selector, size_t argumentCount, beacon_NativeCodeFunction_t primitive);

beacon_oop_t beacon_boxExternalAddress(beacon_context_t *context, void *pointer);
void *beacon_unboxExternalAddress(beacon_context_t *context, beacon_oop_t box);

#ifdef __cplusplus
}
#endif

#endif // BEACON_CONTEXT_H