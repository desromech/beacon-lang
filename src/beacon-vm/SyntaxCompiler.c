#include "beacon-lang/SyntaxCompiler.h"
#include "beacon-lang/Context.h"

beacon_CompiledMethod_t *beacon_compileWorkspaceSyntax(beacon_context_t *context, beacon_ParseTreeNode_t *parseTree)
{
    beacon_BytecodeCodeBuilder_t *bytecodeBuilder = beacon_BytecodeCodeBuilder_new(context);
    beacon_Symbol_t *selector = beacon_internCString(context, "compileWithEnvironment:andBytecodeBuilder:");
    beacon_BytecodeValue_t lastValue = beacon_decodeSmallInteger(beacon_performWithWith(context, parseTree, selector, 0, bytecodeBuilder));

    beacon_BytecodeCode_t *bytecode = beacon_BytecodeCodeBuilder_finish(context, bytecodeBuilder);

    beacon_CompiledMethod_t *compiledMethod = beacon_allocateObjectWithBehavior(context->heap, context->classes.compiledMethodClass, sizeof(beacon_CompiledMethod_t), BeaconObjectKindPointers);
    compiledMethod->super.argumentCount = bytecode->argumentCount;
    compiledMethod->super.bytecodeImplementation = bytecode;
    return compiledMethod;
}

void beacon_context_registerParseTreeCompilationPrimitives(beacon_context_t *context)
{
}
