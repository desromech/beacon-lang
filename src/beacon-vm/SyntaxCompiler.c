#include "beacon-lang/SyntaxCompiler.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Dictionary.h"
#include "beacon-lang/Exceptions.h"
#include <stdio.h>

static beacon_BytecodeValue_t beacon_compileNodeWithEnvironmentAndBytecodeBuilder(beacon_context_t *context, beacon_ParseTreeNode_t *node, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder)
{
    beacon_oop_t arguments[2] = {
        (beacon_oop_t)environment,
        (beacon_oop_t)builder
    };

    return beacon_decodeSmallInteger(beacon_performWithArguments(context, (beacon_oop_t)node, context->roots.compileWithEnvironmentAndBytecodeBuilderSelector, 2, arguments));
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

    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = beacon_BytecodeCodeBuilder_new(context);
    
    beacon_BytecodeValue_t lastValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, parseTree, &fileEnvironment->super, bytecodeBuilder);

    // Ensure that we are returning at least a value.
    beacon_BytecodeCodeBuilder_localReturn(context, bytecodeBuilder, lastValue);

    beacon_BytecodeCode_t *bytecode = beacon_BytecodeCodeBuilder_finish(context, bytecodeBuilder);

    beacon_CompiledMethod_t *compiledMethod = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledMethodClass, sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers);
    compiledMethod->super.argumentCount = bytecode->argumentCount;
    compiledMethod->super.bytecodeImplementation = bytecode;
    return compiledMethod;
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
    beacon_Array_t *literalArray = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, sizeof(beacon_ByteArray_t) + sizeof(beacon_oop_t)*literalArraySize, BeaconObjectKindBytes);

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

static beacon_oop_t beacon_SyntaxCompiler_messageSend(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeMessageSendNode_t *messageSendNode = (beacon_ParseTreeMessageSendNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t receiverValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, messageSendNode->receiver, environment, builder);
    beacon_BytecodeValue_t selectorValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, messageSendNode->selector, environment, builder);
    beacon_BytecodeValue_t resultValue = beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);

    size_t argumentValueCount = messageSendNode->arguments->super.super.super.super.super.header.slotCount;
    BeaconAssert(context, argumentValueCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
    beacon_BytecodeValue_t argumentValues[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS] = {};

    for(size_t i = 0; i < argumentValueCount; ++i)
        argumentValues[i] = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[i] , environment, builder);

    beacon_BytecodeCodeBuilder_sendMessage(context, builder, resultValue, receiverValue, selectorValue, argumentValueCount, argumentValues);
    return beacon_encodeSmallInteger(resultValue);
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

static beacon_oop_t beacon_SyntaxCompiler_identifierReference(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_ParseTreeIdentifierReferenceNode_t *identifierReference = (beacon_ParseTreeIdentifierReferenceNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_oop_t result = beacon_performWithWith(context, (beacon_oop_t)environment, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, identifierReference->identifier, (beacon_oop_t)builder);
    if(result == 0)
        beacon_exception_error(context, "Symbol binding not found.");

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
    if(beacon_BytecodeValue_getType(storage) != BytecodeArgumentTypeTemporary)
        beacon_exception_error(context, "Cannot assign to non temporary variable.");

    beacon_BytecodeCodeBuilder_storeValue(context, builder, storage, valueToStore);
    return beacon_encodeSmallInteger(valueToStore);
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

static beacon_CompiledMethod_t *beacon_SyntaxCompiler_compileMethodNode(beacon_context_t *context, beacon_ParseTreeMethodNode_t *methodNode, beacon_AbstractCompilationEnvironment_t *environment)
{
    beacon_MethodCompilationEnvironment_t *methodEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.methodCompilationEnvironmentClass, sizeof(beacon_MethodCompilationEnvironment_t), BeaconObjectKindPointers);
    methodEnvironment->parent = environment;
    methodEnvironment->dictionary = beacon_MethodDictionary_new(context);

    beacon_BytecodeCodeBuilder_t *methodBuilder = beacon_BytecodeCodeBuilder_new(context);

    // Method receiver.
    beacon_BytecodeValue_t self = beacon_BytecodeCodeBuilder_getOrCreateSelf(context, methodBuilder);
    beacon_MethodDictionary_atPut(context, methodEnvironment->dictionary, beacon_internCString(context, "self"), beacon_encodeSmallInteger(self));

    // Method arguments
    size_t methodArgumentCount = methodNode->arguments->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < methodArgumentCount; ++i)
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

    beacon_BytecodeValue_t lastValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, methodNode->expression, &methodEnvironment->super, methodBuilder);

    // Ensure that we are at least returning the receiver.
    beacon_BytecodeCodeBuilder_localReturn(context, methodBuilder, lastValue);

    beacon_BytecodeCode_t *bytecode = beacon_BytecodeCodeBuilder_finish(context, methodBuilder);

    beacon_CompiledMethod_t *compiledMethod = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledMethodClass, sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers);
    compiledMethod->name = methodNode->selector;
    compiledMethod->super.argumentCount = bytecode->argumentCount;
    compiledMethod->super.bytecodeImplementation = bytecode;

    return compiledMethod;
}

static beacon_oop_t beacon_SyntaxCompiler_methodNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *parentBuilder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_CompiledMethod_t *compiledMethod = beacon_SyntaxCompiler_compileMethodNode(context, (beacon_ParseTreeMethodNode_t*)receiver, environment);
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

    beacon_CompiledMethod_t *compiledMethod = beacon_SyntaxCompiler_compileMethodNode(context, (beacon_ParseTreeMethodNode_t*)addMethodNode->method, &behaviorEnvironment->super);
    beacon_Behavior_t *targetBehavior = (beacon_Behavior_t *)behavior;
    if(!targetBehavior->methodDict)
        targetBehavior->methodDict = beacon_MethodDictionary_new(context);
    beacon_MethodDictionary_atPut(context, targetBehavior->methodDict, compiledMethod->name, (beacon_oop_t)compiledMethod);

    return beacon_encodeSmallInteger(beacon_BytecodeCodeBuilder_addLiteral(context, builder, behavior));
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

void beacon_context_registerParseTreeCompilationPrimitives(beacon_context_t *context)
{   
    beacon_addPrimitiveToClass(context, context->classes.emptyCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_EmptyCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.systemCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_SystemCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.fileCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_FileCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.lexicalCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_LexicalCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);
    beacon_addPrimitiveToClass(context, context->classes.methodCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_MethodCompilationEnvironment_lookupSymbolRecursivelyWithCodeBuilder);

    beacon_addPrimitiveToClass(context, context->classes.emptyCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_EmptyCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.systemCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_SystemCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.fileCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_FileCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.lexicalCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_LexicalCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.methodCompilationEnvironmentClass, "lookupSymbolRecursively:", 1, beacon_MethodCompilationEnvironment_lookupSymbolRecursively);

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
    beacon_addPrimitiveToClass(context, context->classes.parseTreeMethodNode, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_methodNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeAddMethodNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_addMethodNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeByteArrayNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_byteArrayNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralArrayNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_literalArrayNode);

    beacon_addPrimitiveToClass(context, context->classes.parseTreeNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateLiteralNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeIdentifierReferenceNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateIdentifierReference);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeByteArrayNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateByteArray);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralArrayNodeClass, "evaluateWithEnvironment:", 1, beacon_SyntaxCompiler_evaluateLiteralArray);

        /*

        beacon_Behavior_t *parseTreeArrayNodeClass;
        beacon_Behavior_t *parseTreeLiteralArrayNodeClass;
        beacon_Behavior_t *parseTreeBlockClosureNodeClass;
        */

}
