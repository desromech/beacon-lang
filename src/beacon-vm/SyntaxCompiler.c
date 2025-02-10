#include "beacon-lang/SyntaxCompiler.h"
#include "beacon-lang/Context.h"

beacon_CompiledMethod_t *beacon_compileWorkspaceSyntax(beacon_context_t *context, beacon_ParseTreeNode_t *parseTree)
{
    beacon_EmptyCompilationEnvironment_t *emptyEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.emptyCompilationEnvironmentClass, sizeof(beacon_EmptyCompilationEnvironment_t), BeaconObjectKindPointers);
    beacon_SystemCompilationEnvironment_t *systemEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.systemCompilationEnvironmentClass, sizeof(beacon_SystemCompilationEnvironment_t), BeaconObjectKindPointers);
    systemEnvironment->parent = &emptyEnvironment->super;
    systemEnvironment->systemDictionary = context->roots.systemDictionary;

    beacon_LexicalCompilationEnvironment_t *lexicalEnvironment = beacon_allocateObjectWithBehavior(context->heap, context->classes.lexicalCompilationEnvironmentClass, sizeof(beacon_LexicalCompilationEnvironment_t), BeaconObjectKindPointers);
    lexicalEnvironment->parent = &systemEnvironment->super;
    
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = beacon_BytecodeCodeBuilder_new(context);
    beacon_BytecodeValue_t lastValue = beacon_decodeSmallInteger(beacon_performWithWith(context, (beacon_oop_t)parseTree, context->roots.compileWithEnvironmentAndBytecodeBuilderSelector, (beacon_oop_t)lexicalEnvironment, (beacon_oop_t)bytecodeBuilder));

    beacon_BytecodeCode_t *bytecode = beacon_BytecodeCodeBuilder_finish(context, bytecodeBuilder);

    beacon_CompiledMethod_t *compiledMethod = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledMethodClass, sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers);
    compiledMethod->super.argumentCount = bytecode->argumentCount;
    compiledMethod->super.bytecodeImplementation = bytecode;
    return compiledMethod;
}

static beacon_oop_t beacon_SyntaxCompiler_node(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    assert(argumentCount == 2);
    fprintf(stderr, "TODO: Subclass responsibility");
    abort();   
}

static beacon_oop_t beacon_SyntaxCompiler_literal(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    assert(argumentCount == 2);
    beacon_ParseTreeLiteralNode_t *literalNode = (beacon_ParseTreeLiteralNode_t *)receiver;
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = (beacon_BytecodeCodeBuilder_t *)arguments[1];

    fprintf(stderr, "TODO");
    abort();   
}

void beacon_context_registerParseTreeCompilationPrimitives(beacon_context_t *context)
{
    
    beacon_addPrimitiveToClass(context, context->classes.parseTreeNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_node);
    beacon_addPrimitiveToClass(context, context->classes.parseTreeLiteralNodeClass, "compileWithEnvironment:andBytecodeBuilder:", 2, beacon_SyntaxCompiler_literal);
        /*
        beacon_Behavior_t *parseTreeNodeClass;
        beacon_Behavior_t *parseTreeErrorNodeClass;
        
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
        */

}
