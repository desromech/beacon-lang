#include "beacon-lang/SyntaxCompiler.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Dictionary.h"
#include <stdio.h>
#include <assert.h>

static beacon_BytecodeValue_t beacon_compileNodeWithEnvironmentAndBytecodeBuilder(beacon_context_t *context, beacon_ParseTreeNode_t *node, beacon_AbstractCompilationEnvironment_t *environment, beacon_BytecodeCodeBuilder_t *builder)
{
    beacon_oop_t arguments[2] = {
        (beacon_oop_t)environment,
        (beacon_oop_t)builder
    };

    return beacon_decodeSmallInteger(beacon_performWithArguments(context, (beacon_oop_t)node, context->roots.compileWithEnvironmentAndBytecodeBuilderSelector, 2, arguments));
}

beacon_CompiledMethod_t *beacon_compileReplSyntax(beacon_context_t *context, beacon_ParseTreeNode_t *parseTree)
{
    beacon_EmptyCompilationEnvironment_t *emptyEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.emptyCompilationEnvironmentClass, sizeof(beacon_EmptyCompilationEnvironment_t), BeaconObjectKindPointers);
    beacon_SystemCompilationEnvironment_t *systemEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.systemCompilationEnvironmentClass, sizeof(beacon_SystemCompilationEnvironment_t), BeaconObjectKindPointers);
    systemEnvironment->parent = &emptyEnvironment->super;
    systemEnvironment->systemDictionary = context->roots.systemDictionary;

    beacon_LexicalCompilationEnvironment_t *lexicalEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.lexicalCompilationEnvironmentClass, sizeof(beacon_LexicalCompilationEnvironment_t), BeaconObjectKindPointers);
    lexicalEnvironment->parent = &systemEnvironment->super;
    
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = beacon_BytecodeCodeBuilder_new(context);
    
    beacon_BytecodeValue_t lastValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, parseTree, &lexicalEnvironment->super, bytecodeBuilder);

    // Ensure that we are returning at least a value.
    beacon_BytecodeCodeBuilder_localReturn(context, bytecodeBuilder, lastValue);

    beacon_BytecodeCode_t *bytecode = beacon_BytecodeCodeBuilder_finish(context, bytecodeBuilder);

    beacon_CompiledMethod_t *compiledMethod = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledMethodClass, sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers);
    compiledMethod->super.argumentCount = bytecode->argumentCount;
    compiledMethod->super.bytecodeImplementation = bytecode;
    return compiledMethod;
}

static beacon_oop_t beacon_SyntaxCompiler_node(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)context;
    (void)receiver;
    (void)arguments;
    assert(argumentCount == 2);
    fprintf(stderr, "TODO: Subclass responsibility");
    abort();   
}

static beacon_oop_t beacon_SyntaxCompiler_literal(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    assert(argumentCount == 2);
    beacon_ParseTreeLiteralNode_t *literalNode = (beacon_ParseTreeLiteralNode_t *)receiver;
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t value = beacon_BytecodeCodeBuilder_addLiteral(context, bytecodeBuilder, literalNode->value);
    return beacon_encodeSmallInteger(value);
}

static beacon_oop_t beacon_SyntaxCompiler_messageSend(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    assert(argumentCount == 2);
    beacon_ParseTreeMessageSendNode_t *messageSendNode = (beacon_ParseTreeMessageSendNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t receiverValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, messageSendNode->receiver, environment, builder);
    beacon_BytecodeValue_t selectorValue = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, messageSendNode->selector, environment, builder);
    beacon_BytecodeValue_t resultValue = beacon_BytecodeCodeBuilder_newTemporary(context, builder, 0);

    size_t argumentValueCount = messageSendNode->arguments->super.super.super.super.super.header.slotCount;
    assert(argumentValueCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
    beacon_BytecodeValue_t argumentValues[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS] = {};

    for(size_t i = 0; i < argumentValueCount; ++i)
        argumentValues[i] = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, (beacon_ParseTreeNode_t*)messageSendNode->arguments->elements[i] , environment, builder);

    beacon_BytecodeCodeBuilder_sendMessage(context, builder, resultValue, receiverValue, selectorValue, argumentValueCount, argumentValues);
    return beacon_encodeSmallInteger(resultValue);
}

static beacon_oop_t beacon_SyntaxCompiler_messageCascade(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    assert(argumentCount == 2);
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
        assert(argumentValueCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
        beacon_BytecodeValue_t argumentValues[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS] = {};

        for(size_t i = 0; i < argumentValueCount; ++i)
            argumentValues[i] = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, (beacon_ParseTreeNode_t*)cascadedMessage->arguments->elements[i] , environment, builder);

        beacon_BytecodeCodeBuilder_sendMessage(context, builder, resultTemporary, receiverValue, selectorValue, argumentValueCount, argumentValues);

    }
    return beacon_encodeSmallInteger(resultTemporary);
}

static beacon_oop_t beacon_SyntaxCompiler_identifierReference(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    assert(argumentCount == 2);
    beacon_ParseTreeIdentifierReferenceNode_t *identifierReference = (beacon_ParseTreeIdentifierReferenceNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_oop_t result = beacon_performWithWith(context, (beacon_oop_t)environment, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, identifierReference->identifier, (beacon_oop_t)builder);
    if(result == 0)
    {
        fprintf(stderr, "Symbol binding not found.");
        abort();
    }

    return result;
}

static beacon_oop_t beacon_SyntaxCompiler_sequenceNode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    assert(argumentCount == 2);
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
    assert(argumentCount == 2);
    beacon_ParseTreeReturnNode_t *returnNode = (beacon_ParseTreeReturnNode_t *)receiver;
    beacon_AbstractCompilationEnvironment_t *environment = (beacon_AbstractCompilationEnvironment_t*)arguments[0];
    beacon_BytecodeCodeBuilder_t *builder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    beacon_BytecodeValue_t result = beacon_compileNodeWithEnvironmentAndBytecodeBuilder(context, returnNode->expression, environment, builder);
    beacon_BytecodeCodeBuilder_localReturn(context, builder, result);
    return  beacon_encodeSmallInteger(0);
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
    assert(argumentCount == 2);
    beacon_SystemCompilationEnvironment_t *environment = (beacon_SystemCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t*)arguments[1];

    if(environment->systemDictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->systemDictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->systemDictionary, symbolToSearch))
            return beacon_encodeSmallInteger(beacon_BytecodeCodeBuilder_addLiteral(context, bytecodeBuilder, oop));
    }

    assert(environment->parent);
    return beacon_performWithWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, (beacon_oop_t)symbolToSearch, (beacon_oop_t)bytecodeBuilder);
}

static beacon_oop_t beacon_LexicalCompilationEnvironment_lookupSymbolRecursively(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    assert(argumentCount == 2);
    beacon_LexicalCompilationEnvironment_t *environment = (beacon_LexicalCompilationEnvironment_t*)receiver;
    beacon_Symbol_t *symbolToSearch = (beacon_Symbol_t *)arguments[0];
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t*)arguments[1];

    if(environment->dictionary)
    {
        beacon_oop_t oop = beacon_MethodDictionary_atOrNil(context, environment->dictionary, symbolToSearch);
        if(oop || beacon_MethodDictionary_includesKey(context, environment->dictionary, symbolToSearch))
            return oop;
    }

    assert(environment->parent);
    return beacon_performWithWith(context, (beacon_oop_t)environment->parent, context->roots.lookupSymbolRecursivelyWithBytecodeBuilderSelector, (beacon_oop_t)symbolToSearch, (beacon_oop_t)bytecodeBuilder);
}

void beacon_context_registerParseTreeCompilationPrimitives(beacon_context_t *context)
{   
    beacon_addPrimitiveToClass(context, context->classes.emptyCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_EmptyCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.systemCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_SystemCompilationEnvironment_lookupSymbolRecursively);
    beacon_addPrimitiveToClass(context, context->classes.lexicalCompilationEnvironmentClass, "lookupSymbolRecursively:withBytecodeBuilder:", 2, beacon_LexicalCompilationEnvironment_lookupSymbolRecursively);

    beacon_addPrimitiveToClass(context, context->classes.parseTreeNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_node);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_literal);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeMessageSendNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_messageSend);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeMessageCascadeNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_messageCascade);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeIdentifierReferenceNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_identifierReference);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeReturnNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_returnNode);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeSequenceNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_sequenceNode);
        /*
        beacon_Behavior_t *parseTreeErrorNodeClass;
        
        beacon_Behavior_t *parseTreeMessageCascadeNodeClass;
        beacon_Behavior_t *parseTreeCascadedMessageNodeClass;
        beacon_Behavior_t *parseTreeArrayNodeClass;
        beacon_Behavior_t *parseTreeLiteralArrayNodeClass;
        beacon_Behavior_t *parseTreeArgumentDefinitionNodeClass;
        beacon_Behavior_t *parseTreeLocalVariableDefinitionNodeClass;
        beacon_Behavior_t *parseTreeBlockClosureNodeClass;
        */

}
