#include "beacon-lang/SyntaxCompiler.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Dictionary.h"
#include "beacon-lang/Exceptions.h"
#include "beacon-lang/ArrayList.h"
#include <stdlib.h>
#include <stdio.h>

static beacon_BytecodeValue_t beacon_compileNodeWithEnvironmentAndBytecodeBuilder(beacon_context_t *context, beacon_ParseTreeNode_t *node, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder)
{
    beacon_oop_t arguments[2] = {
        (beacon_oop_t)environment,
        (beacon_oop_t)builder
    };

    return beacon_decodeSmallInteger(beacon_performWithArguments(context, (beacon_oop_t)node, context->roots.compileWithEnvironmentAndBytecodeBuilderSelector, 2, arguments));
}

static beacon_oop_t beacon_evaluateNodeWithEnvironment(beacon_context_t *context, beacon_ParseTreeNode_t *node, beacon_AbstractCompilationEnvironment_t *environment)
{
    beacon_oop_t arguments[] = {
        (beacon_oop_t)environment
    };

    return beacon_performWithArguments(context, (beacon_oop_t)node, context->roots.evaluateWithEnvironmentSelector, 1, arguments);
}

static beacon_BytecodeValue_t beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(beacon_context_t *context, beacon_ParseTreeNode_t *node, beacon_Array_t *blockArguments, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder)
{
    beacon_oop_t arguments[3] = {
        (beacon_oop_t)blockArguments,
        (beacon_oop_t)environment,
        (beacon_oop_t)builder
    };

    return beacon_decodeSmallInteger(beacon_performWithArguments(context, (beacon_oop_t)node, context->roots.compileInlineBlockWithArgumentsWithEnvironmentAndBytecodeBuilderSelector, 3, arguments));
}

beacon_CompiledMethod_t *beacon_compileFileSyntax(beacon_context_t *context, beacon_ParseTreeNode_t *parseTree, beacon_SourceCode_t *sourceCode)
{
    beacon_EmptyCompilationEnvironment_t *emptyEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.emptyCompilationEnvironmentClass, sizeof(beacon_EmptyCompilationEnvironment_t), BeaconObjectKindPointers);
    beacon_SystemCompilationEnvironment_t *systemEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.systemCompilationEnvironmentClass, sizeof(beacon_SystemCompilationEnvironment_t), BeaconObjectKindPointers);
    beacon_FileCompilationEnvironment_t *fileEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.fileCompilationEnvironmentClass, sizeof(beacon_FileCompilationEnvironment_t), BeaconObjectKindPointers);

    systemEnvironment->parent = &emptyEnvironment->super;
    systemEnvironment->systemDictionary = context->roots.systemDictionary;

    fileEnvironment->parent = &systemEnvironment->super;
    fileEnvironment->dictionary = beacon_MethodDictionary_new(context);
    beacon_MethodDictionary_atPut(context, fileEnvironment->dictionary, beacon_internCString(context, "__FileDir__"), (beacon_oop_t)sourceCode->directory);
    beacon_MethodDictionary_atPut(context, fileEnvironment->dictionary, beacon_internCString(context, "__FileName__"), (beacon_oop_t)sourceCode->name);

    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = beacon_BytecodeCodeBuilder_new(context, NULL);
    
    beacon_BytecodeValue_t lastValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, parseTree, &fileEnvironment->super, bytecodeBuilder);

    // Ensure that we are returning at least a value.
    beacon_BytecodeCodeBuilder_localReturn(context, bytecodeBuilder, lastValue);

    beacon_BytecodeCode_t *bytecode = beacon_BytecodeCodeBuilder_finish(context, bytecodeBuilder);

    beacon_CompiledMethod_t *compiledMethod = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledMethodClass, sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers);
    compiledMethod->super.argumentCount = bytecode->argumentCount;
    compiledMethod->super.bytecodeImplementation = bytecode;
    compiledMethod->super.sourcePosition = parseTree->sourcePosition;
    return compiledMethod;
}

beacon_oop_t beacon_evaluateFileSyntax(beacon_context_t *context, beacon_ParseTreeNode_t *parseTree, beacon_SourceCode_t *sourceCode)
{
    beacon_EmptyCompilationEnvironment_t *emptyEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.emptyCompilationEnvironmentClass, sizeof(beacon_EmptyCompilationEnvironment_t), BeaconObjectKindPointers);
    beacon_SystemCompilationEnvironment_t *systemEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.systemCompilationEnvironmentClass, sizeof(beacon_SystemCompilationEnvironment_t), BeaconObjectKindPointers);
    beacon_FileCompilationEnvironment_t *fileEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.fileCompilationEnvironmentClass, sizeof(beacon_FileCompilationEnvironment_t), BeaconObjectKindPointers);

    systemEnvironment->parent = &emptyEnvironment->super;
    systemEnvironment->systemDictionary = context->roots.systemDictionary;

    fileEnvironment->parent = &systemEnvironment->super;
    fileEnvironment->dictionary = beacon_MethodDictionary_new(context);
    beacon_MethodDictionary_atPut(context, fileEnvironment->dictionary, beacon_internCString(context, "__FileDir__"), (beacon_oop_t)sourceCode->directory);
    beacon_MethodDictionary_atPut(context, fileEnvironment->dictionary, beacon_internCString(context, "__FileName__"), (beacon_oop_t)sourceCode->name);
    
    beacon_oop_t result = beacon_evaluateNodeWithEnvironment(context, parseTree, &fileEnvironment->super);
    return result;
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)receiver;
    (void)arguments;
    BeaconAssert(context, argumentCount == 1);
    beacon_exception_subclassResponsibility(context, receiver, context->roots.evaluateWithEnvironmentSelector);
    return 0;
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateLiteralNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 1);
    beacon_ParseTreeLiteralNode_t *literalNode = (beacon_ParseTreeLiteralNode_t *)receiver;
    return literalNode->value;
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateIdentifierReference(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 1);
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_ParseTreeIdentifierReferenceNode_t *identifierReferenceNode = (beacon_ParseTreeIdentifierReferenceNode_t*)receiver;
    beacon_oop_t result = beacon_performWith(context, (beacon_oop_t)environment, context->roots.lookupSymbolRecursivelySelector, identifierReferenceNode->identifier);
    if(beacon_getClass(context, result) == context->classes.associationClass)
    {
        beacon_Association_t *assoc = (beacon_Association_t*)result;
        return assoc->value;
    }
    return result;
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateByteArray(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 1);
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_ParseTreeByteArrayNode_t *byteArrayNode = (beacon_ParseTreeByteArrayNode_t*)receiver;
    size_t byteArraySize = byteArrayNode->elements->super.super.super.super.super.header.slotCount;
    beacon_ByteArray_t *byteArray = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, sizeof(beacon_ByteArray_t) + byteArraySize, BeaconObjectKindBytes);
    for(size_t i = 0; i < byteArraySize; ++i)
    {
        beacon_oop_t integerNode = byteArrayNode->elements->elements[i];
        beacon_oop_t integerValue = beacon_performWith(context, integerNode, context->roots.evaluateWithEnvironmentSelector, (beacon_oop_t)environment);
        byteArray->elements[i] = beacon_decodeSmallInteger(integerValue);
    }

    return (beacon_oop_t)byteArray;
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateLiteralArray(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)arguments;
    BeaconAssert(context, argumentCount == 1);
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_ParseTreeByteArrayNode_t *literalArrayNode = (beacon_ParseTreeByteArrayNode_t*)receiver;
    size_t literalArraySize = literalArrayNode->elements->super.super.super.super.super.header.slotCount;
    beacon_Array_t *literalArray = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t)*literalArraySize, BeaconObjectKindPointers);

    for(size_t i = 0; i < literalArraySize; ++i)
    {
        beacon_oop_t elementNode = literalArrayNode->elements->elements[i];
        beacon_oop_t elementValue = beacon_performWith(context, elementNode, context->roots.evaluateWithEnvironmentSelector, (beacon_oop_t)environment);
        literalArray->elements[i] = elementValue;
    }

    return (beacon_oop_t)literalArray;
}

static beacon_oop_t beacon_SyntaxCompiler_node(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)receiver;
    (void)arguments;
    BeaconAssert(context, argumentCount == 2);
    beacon_exception_subclassResponsibility(context, receiver, context->roots.compileWithEnvironmentAndBytecodeBuilderSelector);
    return 0;
}

static beacon_oop_t beacon_SyntaxCompiler_error(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    beacon_ParseTreeErrorNode_t *errorNode = (beacon_ParseTreeErrorNode_t*)receiver;
    (void)arguments;
    BeaconAssert(context, argumentCount == 2);
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Parse error %.*s\n", errorNode->errorMessage->super.super.super.super.super.header.slotCount, errorNode->errorMessage->data);
    beacon_exception_error(context, buffer);
    return 0;
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateWorkspaceScript(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_ParseTreeWorkspaceScriptNode_t *scriptNode = (beacon_ParseTreeWorkspaceScriptNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_LexicalCompilationEnvironment_t *lexicalEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.lexicalCompilationEnvironmentClass, sizeof(beacon_LexicalCompilationEnvironment_t), BeaconObjectKindPointers);
    lexicalEnvironment->parent = environment;
    
    beacon_StackFrameRecord_t frameRecord = {
        .kind = StackFramePrimitiveRoots,
        .context = context,
        .primitiveRoots = {
            .receiver = receiver,
            .argumentCount = argumentCount,
            .arguments = arguments,
            .allocatedObjects = {
                (beacon_oop_t)lexicalEnvironment
            }
        }
    };
    beacon_pushStackFrameRecord(&frameRecord);

    size_t localVariableCount = scriptNode->localVariables->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < localVariableCount; ++i)
    {
        beacon_ParseTreeLocalVariableDefinitionNode_t *definition = (beacon_ParseTreeLocalVariableDefinitionNode_t*)scriptNode->localVariables->elements[i];
        beacon_Association_t *assoc = beacon_allocateObjectWithBehavior(context->heap, context->classes.associationClass, sizeof(beacon_Association_t), BeaconObjectKindPointers);
        assoc->key = (beacon_oop_t)definition->name;

        if(!lexicalEnvironment->dictionary)
            lexicalEnvironment->dictionary = beacon_MethodDictionary_new(context);
        
        beacon_MethodDictionary_atPut(context, lexicalEnvironment->dictionary, definition->name, (beacon_oop_t)assoc);
    }

    frameRecord.primitiveRoots.result = beacon_evaluateNodeWithEnvironment(context, scriptNode->expression, &lexicalEnvironment->super);
    beacon_popStackFrameRecord(&frameRecord);
    return frameRecord.primitiveRoots.result;
}


static beacon_oop_t beacon_SyntaxCompiler_workspaceScript(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeWorkspaceScriptNode_t *scriptNode = (beacon_ParseTreeWorkspaceScriptNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_LexicalCompilationEnvironment_t *lexicalEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.lexicalCompilationEnvironmentClass, sizeof(beacon_LexicalCompilationEnvironment_t), BeaconObjectKindPointers);
    lexicalEnvironment->parent = environment;

    size_t localVariableCount = scriptNode->localVariables->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < localVariableCount; ++i)
    {
        beacon_ParseTreeLocalVariableDefinitionNode_t *definition = (beacon_ParseTreeLocalVariableDefinitionNode_t*)scriptNode->localVariables->elements[i];
        beacon_BytecodeValue_t temporaryValue = beacon_BytecodeCodeBuilder_newTemporary(context, builder, (beacon_oop_t)definition->name);

        if(!lexicalEnvironment->dictionary)
            lexicalEnvironment->dictionary = beacon_MethodDictionary_new(context);
        
        beacon_MethodDictionary_atPut(context, lexicalEnvironment->dictionary, definition->name, beacon_encodeSmallInteger(temporaryValue));        
    }

    beacon_BytecodeValue_t resultValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, scriptNode->expression, &lexicalEnvironment->super, builder);

    return beacon_encodeSmallInteger(resultValue);
}

static beacon_oop_t beacon_SyntaxCompiler_literal(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeLiteralNode_t *literalNode = (beacon_ParseTreeLiteralNode_t *)receiver;
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t value = beacon_BytecodeCodeBuilder_addLiteral(context, bytecodeBuilder, literalNode->value);
    return beacon_encodeSmallInteger(value);
}

static beacon_oop_t beacon_SyntaxCompiler_ifTrue(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_BytecodeValue_t receiver, beacon_ParseTreeNode_t *inlineableArgument)
{
    uint16_t branchLocation = beacon_BytecodeCodeBuilder_jumpIfFalse(context, builder, receiver, 0);

    beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, inlineableArgument, (beacon_Array_t*)context->roots.emptyArray, environment, builder);

    uint16_t mergeLocation = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, branchLocation, mergeLocation);
    return 0;
}

static beacon_oop_t beacon_SyntaxCompiler_ifFalse(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_BytecodeValue_t receiver, beacon_ParseTreeNode_t *inlineableArgument)
{
    uint16_t branchLocation = beacon_BytecodeCodeBuilder_jumpIfTrue(context, builder, receiver, 0);

    beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, inlineableArgument, (beacon_Array_t*)context->roots.emptyArray, environment, builder);

    uint16_t mergeLocation = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, branchLocation, mergeLocation);
    return 0;
}

static beacon_oop_t beacon_SyntaxCompiler_ifTrueIfFalse(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_BytecodeValue_t receiver, beacon_ParseTreeNode_t *firstInlineableArgument, beacon_ParseTreeNode_t *secondInlineableArgument)
{
    beacon_BytecodeValue_t result =  beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);;
    uint16_t firstBranchLocation = beacon_BytecodeCodeBuilder_jumpIfFalse(context, builder, receiver, 0);

    // ifTrue:
    beacon_BytecodeValue_t trueResult = beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, firstInlineableArgument, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    beacon_BytecodeCodeBuilder_storeValue(context, builder, result, trueResult);
    uint16_t jumpMergeLocation = beacon_BytecodeCodeBuilder_jump(context, builder, 0);

    // ifFalse:
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, firstBranchLocation, beacon_BytecodeCodeBuilder_label(builder));
    beacon_BytecodeValue_t falseResult = beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, secondInlineableArgument, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    beacon_BytecodeCodeBuilder_storeValue(context, builder, result, falseResult);

    // Merge
    uint16_t mergeLocation = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jump(context, builder, jumpMergeLocation, mergeLocation);
    
    return beacon_encodeSmallInteger(result);
}

static beacon_oop_t beacon_SyntaxCompiler_ifFalseIfTrue(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_BytecodeValue_t receiver, beacon_ParseTreeNode_t *firstInlineableArgument, beacon_ParseTreeNode_t *secondInlineableArgument)
{
    beacon_BytecodeValue_t result =  beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);
    uint16_t firstBranchLocation = beacon_BytecodeCodeBuilder_jumpIfTrue(context, builder, receiver, 0);

    // ifFalse:
    beacon_BytecodeValue_t falseResult = beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, firstInlineableArgument, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    beacon_BytecodeCodeBuilder_storeValue(context, builder, result, falseResult);
    uint16_t jumpMergeLocation = beacon_BytecodeCodeBuilder_jump(context, builder, 0);

    // True:
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, firstBranchLocation, beacon_BytecodeCodeBuilder_label(builder));
    beacon_BytecodeValue_t trueResult = beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, secondInlineableArgument, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    beacon_BytecodeCodeBuilder_storeValue(context, builder, result, trueResult);

    // Merge
    uint16_t mergeLocation = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jump(context, builder, jumpMergeLocation, mergeLocation);
    
    return beacon_encodeSmallInteger(result);
}

static beacon_oop_t beacon_SyntaxCompiler_and(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_BytecodeValue_t receiver, beacon_ParseTreeNode_t *inlineableArgument)
{
    beacon_BytecodeValue_t result =  beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);
    beacon_BytecodeCodeBuilder_storeValue(context, builder, result, receiver);

    uint16_t branchLocation = beacon_BytecodeCodeBuilder_jumpIfFalse(context, builder, receiver, 0);

    beacon_BytecodeValue_t andNext = beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, inlineableArgument, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    beacon_BytecodeCodeBuilder_storeValue(context, builder, result, andNext);

    uint16_t mergeLocation = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, branchLocation, mergeLocation);
    return beacon_encodeSmallInteger(result);
}

static beacon_oop_t beacon_SyntaxCompiler_or(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_BytecodeValue_t receiver, beacon_ParseTreeNode_t *inlineableArgument)
{
    beacon_BytecodeValue_t result =  beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);
    beacon_BytecodeCodeBuilder_storeValue(context, builder, result, receiver);

    uint16_t branchLocation = beacon_BytecodeCodeBuilder_jumpIfTrue(context, builder, receiver, 0);

    beacon_BytecodeValue_t orNext = beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, inlineableArgument, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    beacon_BytecodeCodeBuilder_storeValue(context, builder, result, orNext);

    uint16_t mergeLocation = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, branchLocation, mergeLocation);
    return beacon_encodeSmallInteger(result);
}

static beacon_oop_t beacon_SyntaxCompiler_toDo(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_BytecodeValue_t receiver, beacon_ParseTreeNode_t *endIndex, beacon_ParseTreeNode_t *inlineableArgument)
{
    beacon_BytecodeValue_t index =  beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);
    beacon_BytecodeValue_t canContinue =  beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);
    beacon_BytecodeValue_t startingIndex = receiver;
    beacon_BytecodeValue_t endingIndexValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, endIndex, environment, builder);

    beacon_BytecodeCodeBuilder_storeValue(context, builder, index, startingIndex);
    uint16_t loopIterationStart = beacon_BytecodeCodeBuilder_label(builder);

    beacon_BytecodeValue_t lessOrEqualsSelector = beacon_BytecodeCodeBuilder_addLiteral(context, builder, context->roots.lessOrEqualsSelector);
    beacon_BytecodeCodeBuilder_sendMessage(context, builder, canContinue, index, lessOrEqualsSelector, 1, &endingIndexValue);
    uint16_t loopEndJump = beacon_BytecodeCodeBuilder_jumpIfFalse(context, builder, canContinue, 0);

    // Inline the block
    beacon_Array_t *blockArgument = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t), BeaconObjectKindPointers);
    blockArgument->elements[0] = beacon_encodeSmallInteger(index);
    beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, inlineableArgument, blockArgument, environment, builder);

    // Increment the index
    beacon_BytecodeValue_t increment = beacon_BytecodeCodeBuilder_addLiteral(context, builder, beacon_encodeSmallInteger(1));
    beacon_BytecodeValue_t plusSelector = beacon_BytecodeCodeBuilder_addLiteral(context, builder, context->roots.plusSelector);
    beacon_BytecodeCodeBuilder_sendMessage(context, builder, index, index, plusSelector, 1, &increment);
    beacon_BytecodeCodeBuilder_jump(context, builder, loopIterationStart);

    // Merge section.
    uint16_t loopMerge = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, loopEndJump, loopMerge);
    return 0;
}

static beacon_oop_t beacon_SyntaxCompiler_whileTrueDo(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_ParseTreeNode_t *receiverBlock, beacon_ParseTreeNode_t *doBlock)
{
    // Loop header
    uint16_t loopHeader = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeValue_t conditionValue = beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, receiverBlock, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    uint16_t headerJump = beacon_BytecodeCodeBuilder_jumpIfFalse(context, builder, conditionValue, 0);

    // Do section.
    if(doBlock)
        beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, doBlock, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    beacon_BytecodeCodeBuilder_jump(context, builder, loopHeader);

    // Merge section
    uint16_t loopMerge = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, headerJump, loopMerge);

    return 0;
}

static beacon_oop_t beacon_SyntaxCompiler_whileFalseDo(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder, beacon_ParseTreeNode_t *receiverBlock, beacon_ParseTreeNode_t *doBlock)
{
    // Loop header
    uint16_t loopHeader = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeValue_t conditionValue = beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, receiverBlock, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    uint16_t headerJump = beacon_BytecodeCodeBuilder_jumpIfTrue(context, builder, conditionValue, 0);

    // Do section.
    if(doBlock)
        beacon_compileInlineNodeWithEnvironmentAndBytecodeBuilder(context, doBlock, (beacon_Array_t*)context->roots.emptyArray, environment, builder);
    beacon_BytecodeCodeBuilder_jump(context, builder, loopHeader);

    // Merge section
    uint16_t loopMerge = beacon_BytecodeCodeBuilder_label(builder);
    beacon_BytecodeCodeBuilder_fixup_jumpIf(context, builder, headerJump, loopMerge);

    return 0;
}

static beacon_oop_t beacon_SyntaxCompiler_messageSend(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeMessageSendNode_t *messageSendNode = (beacon_ParseTreeMessageSendNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_oop_t selectorEvaluatedValue = beacon_performWith(context, (beacon_oop_t)messageSendNode->selector, context->roots.evaluateWithEnvironmentSelector, (beacon_oop_t)environment);
    bool receiverIsBlock = beacon_getClass(context, (beacon_oop_t)messageSendNode->receiver) == context->classes.parseTreeBlockClosureNodeClass;
    size_t argumentValueCount = messageSendNode->arguments->super.super.super.super.super.header.slotCount;
    if(receiverIsBlock)
    {
        if(selectorEvaluatedValue == context->roots.whileTrueSelector)
        {
            BeaconAssert(context, argumentValueCount == 0);
            return beacon_SyntaxCompiler_whileTrueDo(context, environment, builder, messageSendNode->receiver, 0);
        }
        else if(selectorEvaluatedValue == context->roots.whileTrueDoSelector)
        {
            BeaconAssert(context, argumentValueCount == 1);
            return beacon_SyntaxCompiler_whileTrueDo(context, environment, builder, messageSendNode->receiver, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0]);
        }
        if(selectorEvaluatedValue == context->roots.whileFalseSelector)
        {
            BeaconAssert(context, argumentValueCount == 0);
            return beacon_SyntaxCompiler_whileTrueDo(context, environment, builder, messageSendNode->receiver, 0);
        }
        else if(selectorEvaluatedValue == context->roots.whileFalseDoSelector)
        {
            BeaconAssert(context, argumentValueCount == 1);
            return beacon_SyntaxCompiler_whileFalseDo(context, environment, builder, messageSendNode->receiver, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0]);
        }
    }

    beacon_BytecodeValue_t receiverValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, messageSendNode->receiver, environment, builder);
    BeaconAssert(context, argumentValueCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
    bool isSuperSend = beacon_BytecodeValue_getType(receiverValue) == BytecodeArgumentTypeSuperReceiver;
    if(!isSuperSend)
    {
        if(selectorEvaluatedValue == context->roots.ifTrueSelector)
        {
            BeaconAssert(context, argumentValueCount == 1);
            return beacon_SyntaxCompiler_ifTrue(context, environment, builder, receiverValue, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0]);
        }
        else if(selectorEvaluatedValue == context->roots.ifFalseSelector)
        {
            BeaconAssert(context, argumentValueCount == 1);
            return beacon_SyntaxCompiler_ifFalse(context, environment, builder, receiverValue, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0]);
        }
        else if(selectorEvaluatedValue == context->roots.ifTrueIfFalseSelector)
        {
            BeaconAssert(context, argumentValueCount == 2);
            return beacon_SyntaxCompiler_ifTrueIfFalse(context, environment, builder, receiverValue, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0], (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[1]);
        }
        else if(selectorEvaluatedValue == context->roots.ifFalseIfTrueSelector)
        {
            BeaconAssert(context, argumentValueCount == 2);
            return beacon_SyntaxCompiler_ifFalseIfTrue(context, environment, builder, receiverValue, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0], (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[1]);
        }
        else if(selectorEvaluatedValue == context->roots.andSelector)
        {
            BeaconAssert(context, argumentValueCount == 1);
            return beacon_SyntaxCompiler_and(context, environment, builder, receiverValue, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0]);
        }
        else if(selectorEvaluatedValue == context->roots.orSelector)
        {
            BeaconAssert(context, argumentValueCount == 1);
            return beacon_SyntaxCompiler_or(context, environment, builder, receiverValue, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0]);
        }
        else if(selectorEvaluatedValue == context->roots.toDoSelector)
        {
            BeaconAssert(context, argumentValueCount == 2);
            return beacon_SyntaxCompiler_toDo(context, environment, builder, receiverValue, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0], (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[1]);
        }
    }
    

    beacon_BytecodeValue_t selectorValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, messageSendNode->selector, environment, builder);
    beacon_BytecodeValue_t resultValue = beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);

    beacon_BytecodeValue_t argumentValues[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS] = {};

    for(size_t i = 0; i < argumentValueCount; ++i)
        argumentValues[i] = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[i] , environment, builder);

    if(isSuperSend)
        beacon_BytecodeCodeBuilder_superSendMessage(context, builder, resultValue, receiverValue, selectorValue, argumentValueCount, argumentValues);
    else
        beacon_BytecodeCodeBuilder_sendMessage(context, builder, resultValue, receiverValue, selectorValue, argumentValueCount, argumentValues);
    return beacon_encodeSmallInteger(resultValue);
}

static beacon_oop_t beacon_evaluateParseTreeBlockWithValue(beacon_context_t *context, beacon_AbstractCompilationEnvironment_t *environment, beacon_ParseTreeNode_t *blockParseTreeNode, beacon_oop_t argument)
{
    if(beacon_getClass(context, (beacon_oop_t)blockParseTreeNode) != context->classes.parseTreeBlockClosureNodeClass)
        return beacon_evaluateNodeWithEnvironment(context, blockParseTreeNode, environment);

    beacon_ParseTreeBlockClosureNode_t *blockClosureNode = (beacon_ParseTreeBlockClosureNode_t*)blockParseTreeNode;
    size_t blockArgumentCount = blockClosureNode->arguments->super.super.super.super.super.header.slotCount;
    if(blockArgumentCount == 0)
        return beacon_evaluateNodeWithEnvironment(context, blockClosureNode->expression, environment);

    BeaconAssert(context, blockArgumentCount == 1);
    beacon_LexicalCompilationEnvironment_t *inlineEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.lexicalCompilationEnvironmentClass, sizeof(beacon_LexicalCompilationEnvironment_t), BeaconObjectKindPointers);
    inlineEnvironment->parent = environment;
    inlineEnvironment->dictionary = beacon_MethodDictionary_new(context);

    beacon_ParseTreeArgumentDefinitionNode_t* argumentDefinition = (beacon_ParseTreeArgumentDefinitionNode_t*) blockClosureNode->arguments->elements[0];
    if(argumentDefinition->name)
        beacon_MethodDictionary_atPut(context, inlineEnvironment->dictionary, argumentDefinition->name, argument);
    
    // Block local variables
    size_t localVariableCount = blockClosureNode->localVariables->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < localVariableCount; ++i)
    {
        beacon_ParseTreeLocalVariableDefinitionNode_t *definition = (beacon_ParseTreeLocalVariableDefinitionNode_t*)blockClosureNode->localVariables->elements[i];
        beacon_Association_t *assoc = beacon_allocateObjectWithBehavior(context->heap, context->classes.associationClass, sizeof(beacon_Association_t), BeaconObjectKindPointers);
        assoc->key = (beacon_oop_t)definition->name;
        beacon_MethodDictionary_atPut(context, inlineEnvironment->dictionary, definition->name, (beacon_oop_t)assoc);
    }

    return beacon_evaluateNodeWithEnvironment(context, blockClosureNode->expression, &inlineEnvironment->super);
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateMessageSend(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_ParseTreeMessageSendNode_t *messageSendNode = (beacon_ParseTreeMessageSendNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_oop_t receiverValue = beacon_evaluateNodeWithEnvironment(context, messageSendNode->receiver, environment);
    beacon_oop_t selectorEvaluatedValue = beacon_evaluateNodeWithEnvironment(context, messageSendNode->selector, environment); 

    size_t argumentValueCount = messageSendNode->arguments->super.super.super.super.super.header.slotCount;
    BeaconAssert(context, argumentValueCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
    
    if(selectorEvaluatedValue == context->roots.ifTrueSelector)
    {
        BeaconAssert(context, argumentValueCount == 1);
        if(receiverValue == context->roots.trueValue)
            return beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0], environment);
        else
            return context->roots.nilValue;
    }
    else if(selectorEvaluatedValue == context->roots.ifFalseSelector)
    {
        BeaconAssert(context, argumentValueCount == 1);
        if(receiverValue == context->roots.falseValue)
            return beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0], environment);
        else
            return context->roots.nilValue;
    }
    else if(selectorEvaluatedValue == context->roots.ifTrueIfFalseSelector)
    {
        BeaconAssert(context, argumentValueCount == 2);
        if(receiverValue == context->roots.trueValue)
            return beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0], environment);
        else
            return beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[1], environment);
    }
    else if(selectorEvaluatedValue == context->roots.ifFalseIfTrueSelector)
    {
        BeaconAssert(context, argumentValueCount == 2);
        if(receiverValue == context->roots.falseValue)
            return beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0], environment);
        else
            return beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[1], environment);
    }
    else if(selectorEvaluatedValue == context->roots.toDoSelector)
    {
        BeaconAssert(context, argumentValueCount == 2);
        BeaconAssert(context, beacon_isImmediate(receiverValue));
        beacon_oop_t stopValueOop = beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[0], environment);
        BeaconAssert(context, beacon_isImmediate(stopValueOop));

        intptr_t startingValue = beacon_decodeSmallInteger(receiverValue);
        intptr_t stopValue = beacon_decodeSmallInteger(stopValueOop);
        beacon_ParseTreeNode_t *block = (beacon_ParseTreeNode_t *)messageSendNode->arguments->elements[1];
        for(intptr_t i = startingValue; i <= stopValue; ++i)
            beacon_evaluateParseTreeBlockWithValue(context, environment, block, beacon_encodeSmallInteger(i));
        return context->roots.nilValue;
    }

    beacon_oop_t argumentValues[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS] = {};

    for(size_t i = 0; i < argumentValueCount; ++i)
        argumentValues[i] = beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[i] , environment);

    beacon_oop_t result = beacon_performWithArguments(context, receiverValue, selectorEvaluatedValue, argumentValueCount, argumentValues);
    return result;
}

static beacon_oop_t beacon_SyntaxCompiler_messageCascade(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeMessageCascadeNode_t *messageCascadeNode = (beacon_ParseTreeMessageCascadeNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t receiverValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, messageCascadeNode->receiver, environment, builder);
    size_t cascadedMessageCount = messageCascadeNode->cascadedMessages->super.super.super.super.super.header.slotCount;
    if(cascadedMessageCount == 0)
        return beacon_encodeSmallInteger(receiverValue);

    beacon_BytecodeValue_t resultTemporary = beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);

    for(size_t i = 0; i < cascadedMessageCount; ++i)
    {
        beacon_ParseTreeCascadedMessageNode_t *cascadedMessage = (beacon_ParseTreeCascadedMessageNode_t *)messageCascadeNode->cascadedMessages->elements[i];
        beacon_BytecodeValue_t selectorValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, cascadedMessage->selector, environment, builder);

        size_t argumentValueCount = cascadedMessage->arguments->super.super.super.super.super.header.slotCount;
        BeaconAssert(context, argumentValueCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
        beacon_BytecodeValue_t argumentValues[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS] = {};

        for(size_t i = 0; i < argumentValueCount; ++i)
            argumentValues[i] = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, (beacon_ParseTreeNode_t*)cascadedMessage->arguments->elements[i] , environment, builder);

        beacon_BytecodeCodeBuilder_sendMessage(context, builder, resultTemporary, receiverValue, selectorValue, argumentValueCount, argumentValues);

    }
    return beacon_encodeSmallInteger(resultTemporary);
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateMessageCascade(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_ParseTreeMessageCascadeNode_t *messageCascadeNode = (beacon_ParseTreeMessageCascadeNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_oop_t receiverValue = beacon_evaluateNodeWithEnvironment(context, messageCascadeNode->receiver, environment);
    size_t cascadedMessageCount = messageCascadeNode->cascadedMessages->super.super.super.super.super.header.slotCount;
    if(cascadedMessageCount == 0)
        return receiverValue;

    beacon_oop_t resultValue = receiverValue;
    for(size_t i = 0; i < cascadedMessageCount; ++i)
    {
        beacon_ParseTreeCascadedMessageNode_t *cascadedMessage = (beacon_ParseTreeCascadedMessageNode_t *)messageCascadeNode->cascadedMessages->elements[i];
        beacon_oop_t selectorValue = beacon_evaluateNodeWithEnvironment(context, cascadedMessage->selector, environment);

        size_t argumentValueCount = cascadedMessage->arguments->super.super.super.super.super.header.slotCount;
        BeaconAssert(context, argumentValueCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
        beacon_oop_t argumentValues[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS] = {};

        for(size_t i = 0; i < argumentValueCount; ++i)
            argumentValues[i] = beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)cascadedMessage->arguments->elements[i] , environment);

        resultValue = beacon_performWithArguments(context, receiverValue, selectorValue, argumentValueCount, argumentValues);
    }

    return resultValue;
}

static beacon_oop_t beacon_SyntaxCompiler_identifierReference(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeIdentifierReferenceNode_t *identifierReference = (beacon_ParseTreeIdentifierReferenceNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_oop_t result = beacon_performWithWith(context, (beacon_oop_t)environment, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, identifierReference->identifier, (beacon_oop_t)builder);
    if(result == 0)
    {
        beacon_Symbol_t *symbol = (beacon_Symbol_t *)identifierReference->identifier;
        char errorBuffer[256];
        snprintf(errorBuffer, sizeof(errorBuffer), "Symbol binding not found for #%.*s.", symbol->super.super.super.super.super.header.slotCount, symbol->data);
        beacon_exception_error(context, errorBuffer);
    }

    return result;
}

static beacon_oop_t beacon_SyntaxCompiler_temporaryAssignment(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeAssignment_t *assignmentNode = (beacon_ParseTreeAssignment_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t valueToStore = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, assignmentNode->valueToStore, environment, builder);
    beacon_BytecodeValue_t storage = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, assignmentNode->storage, environment, builder);
    if(beacon_BytecodeValue_getType(storage) != BytecodeArgumentTypeTemporary &&
       beacon_BytecodeValue_getType(storage) != BytecodeArgumentTypeReceiverSlot)
        beacon_exception_error(context, "Cannot assign to non temporary variable, or a receiver slot.");

    beacon_BytecodeCodeBuilder_storeValue(context, builder, storage, valueToStore);
    return beacon_encodeSmallInteger(valueToStore);
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateAssignment(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_ParseTreeAssignment_t *assignmentNode = (beacon_ParseTreeAssignment_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_oop_t valueToStore = beacon_evaluateNodeWithEnvironment(context, assignmentNode->valueToStore, environment);
    BeaconAssert(context, beacon_getClass(context, (beacon_oop_t)assignmentNode->storage) == context->classes.parseTreeIdentifierReferenceNodeClass);

    beacon_ParseTreeIdentifierReferenceNode_t *storageIdentifier = (beacon_ParseTreeIdentifierReferenceNode_t*)assignmentNode->storage;

    beacon_oop_t result = beacon_performWith(context, (beacon_oop_t)environment, context->roots.lookupSymbolRecursivelySelector, storageIdentifier->identifier);
    BeaconAssert(context, beacon_getClass(context, result) == context->classes.associationClass);

    beacon_Association_t *assoc = (beacon_Association_t*)result;
    return assoc->value = valueToStore;
}

static beacon_oop_t beacon_SyntaxCompiler_sequenceNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeSequenceNode_t *sequenceNode = (beacon_ParseTreeSequenceNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t resultValue = 0;
    size_t sequenceElementCount = sequenceNode->elements->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < sequenceElementCount; ++i)
        resultValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, (beacon_ParseTreeNode_t*)sequenceNode->elements->elements[i], environment, builder);

    return beacon_encodeSmallInteger(resultValue);
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateSequenceNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_ParseTreeSequenceNode_t *sequenceNode = (beacon_ParseTreeSequenceNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_oop_t resultValue = 0;
    size_t sequenceElementCount = sequenceNode->elements->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < sequenceElementCount; ++i)
        resultValue = beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)sequenceNode->elements->elements[i], environment);

    return resultValue;
}

static beacon_oop_t beacon_SyntaxCompiler_returnNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeReturnNode_t *returnNode = (beacon_ParseTreeReturnNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t result = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, returnNode->expression, environment, builder);
    beacon_BytecodeCodeBuilder_localReturn(context, builder, result);
    return  beacon_encodeSmallInteger(0);
}

static beacon_CompiledBlock_t *beacon_SyntaxCompiler_compileBlockClosureNode(beacon_context_t *context, beacon_ParseTreeBlockClosureNode_t *blockClosureNode, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *parentBuilder, beacon_ArrayList_t **outCaptureList)
{
    beacon_BlockClosureCompilationEnvironment_t *blockEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.blockClosureCompilationEnvironmentClass, sizeof(beacon_BlockClosureCompilationEnvironment_t), BeaconObjectKindPointers);
    blockEnvironment->parent = environment;
    blockEnvironment->dictionary = beacon_MethodDictionary_new(context);
    blockEnvironment->captureList = beacon_ArrayList_new(context);
    *outCaptureList = blockEnvironment->captureList;

    beacon_BytecodeCodeBuilder_t *blockBuilder = beacon_BytecodeCodeBuilder_new(context, parentBuilder);

    // Block arguments
    size_t blockArguments = blockClosureNode->arguments->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < blockArguments; ++i)
    {
        beacon_ParseTreeArgumentDefinitionNode_t* argumentDefinition = (beacon_ParseTreeArgumentDefinitionNode_t*) blockClosureNode->arguments->elements[i];
        beacon_BytecodeValue_t argumentValue = beacon_BytecodeCodeBuilder_newArgument(context, blockBuilder, (beacon_oop_t)argumentDefinition->name);
        if(argumentDefinition->name)
            beacon_MethodDictionary_atPut(context, blockEnvironment->dictionary, argumentDefinition->name, beacon_encodeSmallInteger(argumentValue));
    }
    
    // Block local variables
    size_t localVariableCount = blockClosureNode->localVariables->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < localVariableCount; ++i)
    {
        beacon_ParseTreeLocalVariableDefinitionNode_t *definition = (beacon_ParseTreeLocalVariableDefinitionNode_t*)blockClosureNode->localVariables->elements[i];
        beacon_BytecodeValue_t temporaryValue = beacon_BytecodeCodeBuilder_newTemporary(context, blockBuilder, (beacon_oop_t)definition->name);
        beacon_MethodDictionary_atPut(context, blockEnvironment->dictionary, definition->name, beacon_encodeSmallInteger(temporaryValue));        
    }

    beacon_BytecodeValue_t lastValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, blockClosureNode->expression, &blockEnvironment->super, blockBuilder);
    beacon_BytecodeCodeBuilder_localReturn(context, blockBuilder, lastValue);

    beacon_BytecodeCode_t *bytecode = beacon_BytecodeCodeBuilder_finish(context, blockBuilder);

    beacon_CompiledBlock_t *compiledBlock = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledBlockClass, sizeof(beacon_CompiledBlock_t), BeaconObjectKindPointers);
    compiledBlock->super.argumentCount = bytecode->argumentCount;
    compiledBlock->super.bytecodeImplementation = bytecode;
    compiledBlock->super.sourcePosition = blockClosureNode->super.sourcePosition;
    compiledBlock->captureCount = beacon_encodeSmallInteger(beacon_ArrayList_size(blockEnvironment->captureList));

    return compiledBlock;
}

static beacon_oop_t beacon_SyntaxCompiler_blockClosure(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *parentBuilder = (beacon_BytecodeCodeBuilder_t *)arguments[1];
    beacon_ArrayList_t *captureList = NULL;

    beacon_CompiledBlock_t *compiledBlock = beacon_SyntaxCompiler_compileBlockClosureNode(context, (beacon_ParseTreeBlockClosureNode_t*)receiver, environment, parentBuilder, &captureList);

    // If capture list 
    size_t captureListSize = beacon_ArrayList_size(captureList);
    if(captureListSize == 0)
    {
        beacon_BlockClosure_t *blockClosure = beacon_allocateObjectWithBehavior(context->heap, context->classes.blockClosureClass, sizeof(beacon_BlockClosure_t), BeaconObjectKindPointers);
        blockClosure->captures = context->roots.emptyArray;
        blockClosure->code = compiledBlock;

        beacon_BytecodeValue_t blockClosureLiteral = beacon_BytecodeCodeBuilder_addLiteral(context, parentBuilder, (beacon_oop_t)blockClosure);
        return beacon_encodeSmallInteger(blockClosureLiteral);
    }

    // Closure with capture values.
    beacon_BytecodeValue_t *captures = calloc(captureListSize, sizeof(beacon_BytecodeValue_t));
    for(size_t i = 0; i < captureListSize; ++i)
        captures[i] = beacon_decodeSmallInteger(beacon_ArrayList_at(context, captureList, i + 1));

    beacon_BytecodeValue_t compiledBlockCodeLiteral = beacon_BytecodeCodeBuilder_addLiteral(context, parentBuilder, (beacon_oop_t)compiledBlock);
    beacon_BytecodeValue_t closureTemporary = beacon_BytecodeCodeBuilder_newTemporary(context, parentBuilder, 0);
    beacon_BytecodeCodeBuilder_makeClosureInstance(context, parentBuilder, closureTemporary, compiledBlockCodeLiteral, captureListSize, captures);

    free(captures);
    return beacon_encodeSmallInteger(closureTemporary);
}

static beacon_oop_t beacon_SyntaxCompiler_compileInlineBlock(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 3);
    beacon_ParseTreeBlockClosureNode_t *blockClosureNode = (beacon_ParseTreeBlockClosureNode_t *)receiver;
    beacon_Array_t *blockArguments = (beacon_Array_t*)arguments[0];
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[1];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[2];

    beacon_LexicalCompilationEnvironment_t *blockEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.lexicalCompilationEnvironmentClass, sizeof(beacon_LexicalCompilationEnvironment_t), BeaconObjectKindPointers);
    blockEnvironment->parent = environment;
    blockEnvironment->dictionary = beacon_MethodDictionary_new(context);

    // Block arguments
    size_t blockArgumentCount = blockClosureNode->arguments->super.super.super.super.super.header.slotCount;
    size_t blockAvailableArgumentCount = blockArguments->super.super.super.super.super.header.slotCount;
    BeaconAssert(context, blockArgumentCount <= blockAvailableArgumentCount);

    for(size_t i = 0; i < blockArgumentCount; ++i)
    {
        beacon_ParseTreeArgumentDefinitionNode_t* argumentDefinition = (beacon_ParseTreeArgumentDefinitionNode_t*) blockClosureNode->arguments->elements[i];
        if(argumentDefinition->name)
            beacon_MethodDictionary_atPut(context, blockEnvironment->dictionary, argumentDefinition->name, blockArguments->elements[i]);
    }
    
    // Block local variables
    size_t localVariableCount = blockClosureNode->localVariables->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < localVariableCount; ++i)
    {
        beacon_ParseTreeLocalVariableDefinitionNode_t *definition = (beacon_ParseTreeLocalVariableDefinitionNode_t*)blockClosureNode->localVariables->elements[i];
        beacon_BytecodeValue_t temporaryValue = beacon_BytecodeCodeBuilder_newTemporary(context, builder, (beacon_oop_t)definition->name);
        beacon_MethodDictionary_atPut(context, blockEnvironment->dictionary, definition->name, beacon_encodeSmallInteger(temporaryValue));        
    }

    beacon_BytecodeValue_t lastValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, blockClosureNode->expression, &blockEnvironment->super, builder);
    return beacon_encodeSmallInteger(lastValue);
}


static beacon_CompiledMethod_t *beacon_SyntaxCompiler_compileMethodNode(beacon_context_t *context, beacon_ParseTreeMethodNode_t *methodNode, beacon_AbstractCompilationEnvironment_t *environment, beacon_oop_t superBehavior)
{
    beacon_MethodCompilationEnvironment_t *methodEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.methodCompilationEnvironmentClass, sizeof(beacon_MethodCompilationEnvironment_t), BeaconObjectKindPointers);
    methodEnvironment->parent = environment;
    methodEnvironment->dictionary = beacon_MethodDictionary_new(context);

    beacon_BytecodeCodeBuilder_t *methodBuilder = beacon_BytecodeCodeBuilder_new(context, NULL);

    // Method receiver.
    beacon_BytecodeValue_t self = beacon_BytecodeCodeBuilder_getOrCreateSelf(context, methodBuilder);
    beacon_MethodDictionary_atPut(context, methodEnvironment->dictionary, beacon_internCString(context, "self"), beacon_encodeSmallInteger(self));

    beacon_BytecodeValue_t super = beacon_BytecodeCodeBuilder_superReceiverClass(context, methodBuilder, superBehavior);
    beacon_MethodDictionary_atPut(context, methodEnvironment->dictionary, beacon_internCString(context, "super"), beacon_encodeSmallInteger(super));
    
    // Method arguments
    size_t blockArguments = methodNode->arguments->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < blockArguments; ++i)
    {
        beacon_ParseTreeArgumentDefinitionNode_t* argumentDefinition = (beacon_ParseTreeArgumentDefinitionNode_t*) methodNode->arguments->elements[i];
        beacon_BytecodeValue_t argumentValue = beacon_BytecodeCodeBuilder_newArgument(context, methodBuilder, (beacon_oop_t)argumentDefinition->name);
        if(argumentDefinition->name)
            beacon_MethodDictionary_atPut(context, methodEnvironment->dictionary, argumentDefinition->name, beacon_encodeSmallInteger(argumentValue));
    }
    
    // Method local variables
    size_t localVariableCount = methodNode->localVariables->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < localVariableCount; ++i)
    {
        beacon_ParseTreeLocalVariableDefinitionNode_t *definition = (beacon_ParseTreeLocalVariableDefinitionNode_t*)methodNode->localVariables->elements[i];
        beacon_BytecodeValue_t temporaryValue = beacon_BytecodeCodeBuilder_newTemporary(context, methodBuilder, (beacon_oop_t)definition->name);
        beacon_MethodDictionary_atPut(context, methodEnvironment->dictionary, definition->name, beacon_encodeSmallInteger(temporaryValue));        
    }

    beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, methodNode->expression, &methodEnvironment->super, methodBuilder);

    // Ensure that we are at least returning the receiver.
    beacon_BytecodeCodeBuilder_localReturn(context, methodBuilder, self);

    beacon_BytecodeCode_t *bytecode = beacon_BytecodeCodeBuilder_finish(context, methodBuilder);

    beacon_CompiledMethod_t *compiledMethod = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledMethodClass, sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers);
    compiledMethod->name = methodNode->selector;
    compiledMethod->super.argumentCount = bytecode->argumentCount;
    compiledMethod->super.bytecodeImplementation = bytecode;
    compiledMethod->super.sourcePosition = methodNode->super.sourcePosition;

    return compiledMethod;
}

static beacon_oop_t beacon_SyntaxCompiler_methodNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *parentBuilder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_CompiledMethod_t *compiledMethod = beacon_SyntaxCompiler_compileMethodNode(context, (beacon_ParseTreeMethodNode_t*)receiver, environment, 0);
    beacon_BytecodeValue_t compiledMethodLiteralValue = beacon_BytecodeCodeBuilder_addLiteral(context, parentBuilder, (beacon_oop_t)compiledMethod);
    return beacon_encodeSmallInteger(compiledMethodLiteralValue);
}

static beacon_oop_t beacon_SyntaxCompiler_addMethodNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeAddMethod_t *addMethodNode = (beacon_ParseTreeAddMethod_t *)receiver;

    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_oop_t behavior = beacon_performWith(context, (beacon_oop_t)addMethodNode->behavior, context->roots.evaluateWithEnvironmentSelector, (beacon_oop_t)environment);
    // TODO: Assert for behavior.

    beacon_BehaviorCompilationEnvironment_t *behaviorEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.behaviorCompilationEnvironmentClass, sizeof(beacon_BehaviorCompilationEnvironment_t), BeaconObjectKindPointers);
    behaviorEnvironment->behavior = (beacon_Behavior_t*)behavior;
    behaviorEnvironment->parent = environment;

    beacon_CompiledMethod_t *compiledMethod = beacon_SyntaxCompiler_compileMethodNode(context, (beacon_ParseTreeMethodNode_t*)addMethodNode->method, &behaviorEnvironment->super, (beacon_oop_t)behaviorEnvironment->behavior->superclass);
    beacon_Behavior_t *targetBehavior = (beacon_Behavior_t *)behavior;
    if(!targetBehavior->methodDict)
        targetBehavior->methodDict = beacon_MethodDictionary_new(context);
    beacon_MethodDictionary_atPut(context, targetBehavior->methodDict, compiledMethod->name, (beacon_oop_t)compiledMethod);

    return beacon_encodeSmallInteger(beacon_BytecodeCodeBuilder_addLiteral(context, builder, behavior));
}

static beacon_oop_t beacon_SyntaxCompiler_evaluateAddMethodNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_ParseTreeAddMethod_t *addMethodNode = (beacon_ParseTreeAddMethod_t *)receiver;

    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];

    beacon_oop_t behaviorValue = beacon_evaluateNodeWithEnvironment(context, (beacon_ParseTreeNode_t*)addMethodNode->behavior, environment);  

    beacon_BehaviorCompilationEnvironment_t *behaviorEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.behaviorCompilationEnvironmentClass, sizeof(beacon_BehaviorCompilationEnvironment_t), BeaconObjectKindPointers);
    behaviorEnvironment->behavior = (beacon_Behavior_t*)behaviorValue;
    behaviorEnvironment->parent = environment;

    beacon_CompiledMethod_t *compiledMethod = beacon_SyntaxCompiler_compileMethodNode(context, (beacon_ParseTreeMethodNode_t*)addMethodNode->method, &behaviorEnvironment->super, (beacon_oop_t)behaviorEnvironment->behavior->superclass);
    beacon_Behavior_t *targetBehavior = (beacon_Behavior_t *)behaviorValue;
    if(!targetBehavior->methodDict)
        targetBehavior->methodDict = beacon_MethodDictionary_new(context);
    beacon_MethodDictionary_atPut(context, targetBehavior->methodDict, compiledMethod->name, (beacon_oop_t)compiledMethod);

    return behaviorValue;
}

static beacon_oop_t beacon_SyntaxCompiler_arrayNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeArrayNode_t *arrayNode = (beacon_ParseTreeArrayNode_t *)receiver;

    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    // Get the element count, and make space for them.
    size_t elementCount = arrayNode->elements->super.super.super.super.super.header.slotCount;
    beacon_BytecodeValue_t *elementValues = calloc(elementCount, sizeof(beacon_BytecodeValue_t *));
    for(size_t i = 0; i < elementCount; ++i)
    {
        beacon_BytecodeValue_t elementValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, (beacon_ParseTreeNode_t*)arrayNode->elements->elements[i], environment, builder);
        elementValues[i] = elementValue;
    }

    beacon_BytecodeValue_t resultValue = beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);

    beacon_BytecodeCodeBuilder_makeArray(context, builder, resultValue, elementCount, elementValues);
    free(elementValues);

    return beacon_encodeSmallInteger(resultValue);
}

static beacon_oop_t beacon_SyntaxCompiler_byteArrayNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeByteArrayNode_t *byteArrayNode = (beacon_ParseTreeByteArrayNode_t *)receiver;

    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_oop_t value = beacon_performWith(context, (beacon_oop_t)byteArrayNode, context->roots.evaluateWithEnvironmentSelector, (beacon_oop_t)environment);
    return beacon_encodeSmallInteger(beacon_BytecodeCodeBuilder_addLiteral(context, builder, value));
}

static beacon_oop_t beacon_SyntaxCompiler_literalArrayNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeLiteralArrayNode_t *literalArrayNode = (beacon_ParseTreeLiteralArrayNode_t*)receiver;

    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_oop_t value = beacon_performWith(context, (beacon_oop_t)literalArrayNode, context->roots.evaluateWithEnvironmentSelector, (beacon_oop_t)environment);
    return beacon_encodeSmallInteger(beacon_BytecodeCodeBuilder_addLiteral(context, builder, value));
}

static beacon_oop_t beacon_EmptyCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)receiver;
    (void)argumentCount;
    (void)arguments;
    return 0;
}

static beacon_oop_t beacon_SystemCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_SystemCompilationEnvironment_t *environment = (beacon_SystemCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t*)arguments[1];

    if(environment->systemDictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->systemDictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->systemDictionary, symbolToSearch))
            return beacon_encodeSmallInteger(beacon_BytecodeCodeBuilder_addLiteral(context, bytecodeBuilder, oop));
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWithWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, (beacon_oop_t)symbolToSearch, (beacon_oop_t)bytecodeBuilder);
}

static beacon_oop_t beacon_FileCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_FileCompilationEnvironment_t *environment = (beacon_FileCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t*)arguments[1];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return beacon_encodeSmallInteger(beacon_BytecodeCodeBuilder_addLiteral(context, bytecodeBuilder, oop));
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWithWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, (beacon_oop_t)symbolToSearch, (beacon_oop_t)bytecodeBuilder);
}


static beacon_oop_t beacon_LexicalCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_LexicalCompilationEnvironment_t *environment = (beacon_LexicalCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t*)arguments[1];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return oop;
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWithWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, (beacon_oop_t)symbolToSearch, (beacon_oop_t)bytecodeBuilder);
}

static beacon_oop_t beacon_BehaviorCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_BehaviorCompilationEnvironment_t *environment = (beacon_BehaviorCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t*)arguments[1];

    beacon_Behavior_t *currentBehavior = environment->behavior;
    while(currentBehavior)
    {
        beacon_Array_t *slots = (beacon_Array_t *)currentBehavior->slots;
        size_t slotCount = slots->super.super.super.super.super.header.slotCount;
        for(size_t i = 0; i < slotCount; ++i)
        {
            beacon_Slot_t *slot = (beacon_Slot_t *)slots->elements[i];
            if(slot->name == (beacon_oop_t)symbolToSearch)
            {
                beacon_BytecodeValue_t result = beacon_BytecodeCodeBuilder_getReceiverSlot(context, bytecodeBuilder, beacon_decodeSmallInteger(slot->index));
                return beacon_encodeSmallInteger(result);
            }
        }
        currentBehavior = currentBehavior->superclass;
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWithWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, (beacon_oop_t)symbolToSearch, (beacon_oop_t)bytecodeBuilder);
}

static beacon_oop_t beacon_MethodCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_MethodCompilationEnvironment_t *environment = (beacon_MethodCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t*)arguments[1];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return oop;
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWithWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, (beacon_oop_t)symbolToSearch, (beacon_oop_t)bytecodeBuilder);
}

static beacon_oop_t beacon_BlockClosureCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_BlockClosureCompilationEnvironment_t *environment = (beacon_BlockClosureCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t*)arguments[1];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return oop;
    }

    BeaconAssert(context, environment->parent);
    BeaconAssert(context, bytecodeBuilder->parentBuilder);
    beacon_BytecodeCodeBuilder_t *parentBuilder = (beacon_BytecodeCodeBuilder_t *)bytecodeBuilder->parentBuilder;

    beacon_oop_t parentResultOop = beacon_performWithWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, (beacon_oop_t)symbolToSearch, (beacon_oop_t)parentBuilder);

    beacon_BytecodeValue_t parentValue = beacon_decodeSmallInteger(parentResultOop);
    beacon_BytecodeValueType_t parentValueType = beacon_BytecodeValue_getType(parentValue);
    uint16_t parentValueIndex = beacon_BytecodeValue_getIndex(parentValue);

    beacon_oop_t result = 0;
    switch(parentValueType)
    {
    case BytecodeArgumentTypeLiteral:
        {
            beacon_oop_t parentLiteral = beacon_ArrayList_at(context, parentBuilder->literals, parentValueIndex);
            return beacon_BytecodeCodeBuilder_addLiteral(context, bytecodeBuilder, parentLiteral);
        }
    case BytecodeArgumentTypeArgument:
    case BytecodeArgumentTypeTemporary:
    case BytecodeArgumentTypeCapture:
    case BytecodeArgumentTypeReceiverSlot:
        {
            beacon_ArrayList_add(context, environment->captureList, parentResultOop);
            beacon_BytecodeValue_t captureValue = beacon_BytecodeCodeBuilder_newCapture(context, bytecodeBuilder);
            result = beacon_encodeSmallInteger(captureValue);
            beacon_MethodDictionary_atPut(context, environment->dictionary, symbolToSearch, result);
        }
        break;

    default:
        BeaconAssert(context, false && "Unexpected bytecode value");

    }
    return result;
}

static beacon_oop_t beacon_EmptyCompilationEnvironment_lookupSymbolRecursively(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)receiver;
    (void)argumentCount;
    (void)arguments;
    return 0;
}

static beacon_oop_t beacon_SystemCompilationEnvironment_lookupSymbolRecursively(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_SystemCompilationEnvironment_t *environment = (beacon_SystemCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];

    if(environment->systemDictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->systemDictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->systemDictionary, symbolToSearch))
            return oop;
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelySelector, (beacon_oop_t)symbolToSearch);
}

static beacon_oop_t beacon_FileCompilationEnvironment_lookupSymbolRecursively(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_FileCompilationEnvironment_t *environment = (beacon_FileCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return oop;
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelySelector, (beacon_oop_t)symbolToSearch);
}

static beacon_oop_t beacon_LexicalCompilationEnvironment_lookupSymbolRecursively(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_LexicalCompilationEnvironment_t *environment = (beacon_LexicalCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return oop;
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelySelector, (beacon_oop_t)symbolToSearch);
}

static beacon_oop_t beacon_MethodCompilationEnvironment_lookupSymbolRecursively(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_MethodCompilationEnvironment_t *environment = (beacon_MethodCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return oop;
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelySelector, (beacon_oop_t)symbolToSearch);
}

static beacon_oop_t beacon_BlockClosureCompilationEnvironment_lookupSymbolRecursively(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_BlockClosureCompilationEnvironment_t *environment = (beacon_BlockClosureCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return oop;
    }

    BeaconAssert(context, environment->parent);
    return beacon_performWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelySelector, (beacon_oop_t)symbolToSearch);
}

void beacon_context_registerParseTreeCompilationPrimitives(beacon_context_t *context)
{   
    beacon_addPrimitiveToClass(context, context->classes.emptyCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_EmptyCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.systemCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_SystemCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.fileCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_FileCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.lexicalCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_LexicalCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.behaviorCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_BehaviorCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.methodCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_MethodCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.blockClosureCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_BlockClosureCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);

    beacon_addPrimitiveToClass(context, context->classes.emptyCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_EmptyCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.systemCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_SystemCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.fileCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_FileCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.lexicalCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_LexicalCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.methodCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_MethodCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.blockClosureCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_BlockClosureCompilationEnvironment_lookupSymbolRecursively);

    beacon_addPrimitiveToClass(context, context->classes.parseTreeNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_node);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeErrorNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_error);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeWorkspaceScriptNode, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_workspaceScript);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_literal);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeMessageSendNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_messageSend);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeMessageCascadeNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_messageCascade);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeIdentifierReferenceNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_identifierReference);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeAssignmentNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_temporaryAssignment);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeReturnNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_returnNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeSequenceNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_sequenceNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeBlockClosureNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_blockClosure);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeMethodNode, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_methodNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeAddMethodNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_addMethodNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeArrayNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_arrayNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeByteArrayNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_byteArrayNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralArrayNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_literalArrayNode);

    beacon_addPrimitiveToClass(context, context->classes.parseTreeNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeWorkspaceScriptNode, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateWorkspaceScript);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateLiteralNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeMessageSendNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateMessageSend);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeMessageCascadeNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateMessageCascade);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeIdentifierReferenceNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateIdentifierReference);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeAssignmentNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateAssignment);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeSequenceNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateSequenceNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralArrayNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateLiteralArray);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeAddMethodNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateAddMethodNode);

    
    beacon_addPrimitiveToClass(context, context->classes.parseTreeBlockClosureNodeClass, "compileInlineBlockWithArguments:environment:andBytecodeBuilder:", 3, beacon_SyntaxCompiler_compileInlineBlock);
}
