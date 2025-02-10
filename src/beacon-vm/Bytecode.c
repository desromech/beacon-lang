#include "beacon-lang/Bytecode.h"
#include "beacon-lang/ArrayList.h"

beacon_BytecodeCodeBuilder_t *beacon_BytecodeCodeBuilder_new(beacon_context_t *context)
{
    beacon_BytecodeCodeBuilder_t *builder = beacon_allocateObjectWithBehavior(context->heap, context->classes.bytecodeCodeBuilderClass, sizeof(beacon_BytecodeCodeBuilder_t), BeaconObjectKindPointers);
    builder->arguments = beacon_ArrayList_new(context);
    builder->temporaries = beacon_ArrayList_new(context);
    builder->literals = beacon_ArrayList_new(context);
    builder->bytecodes = beacon_ByteArrayList_new(context);
    return builder;
}

beacon_BytecodeCode_t *beacon_BytecodeCodeBuilder_finish(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *builder)
{
    beacon_BytecodeCode_t *code = beacon_allocateObjectWithBehavior(context->heap, context->classes.bytecodeCodeClass, sizeof(beacon_BytecodeCode_t), BeaconObjectKindPointers);
    code->argumentCount = beacon_encodeSmallInteger(beacon_ArrayList_size(builder->arguments));
    code->temporaryCount = beacon_encodeSmallInteger(beacon_ArrayList_size(builder->temporaries));
    code->literals = beacon_ArrayList_asArray(context, builder->literals);
    code->bytecodes = beacon_ByteArrayList_asByteArray(context, builder->bytecodes);
    return code;
}

beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_addLiteral(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, beacon_oop_t literal)
{
    size_t literalCount = beacon_ArrayList_size(codeBuilder->literals);

    for(size_t i = 1; i <= literalCount; ++i)
    {
        beacon_oop_t existingLiteral = beacon_ArrayList_at(context, codeBuilder->literals, i);
        if(existingLiteral == literal)
            return beacon_BytecodeValue_encode(i - 1, BytecodeArgumentTypeLiteral);

    }

    beacon_ArrayList_add(context, codeBuilder->literals, literal);
    return beacon_BytecodeValue_encode(beacon_ArrayList_size(codeBuilder->literals) - 1, BytecodeArgumentTypeLiteral);
}

uint16_t beacon_BytecodeCodeBuilder_label(beacon_BytecodeCodeBuilder_t *methodBuilder)
{
    return beacon_ByteArrayList_size(methodBuilder->bytecodes);
}

void beacon_BytecodeCodeBuilder_nop(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder)
{
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, BeaconBytecodeNop);
}

void beacon_BytecodeCodeBuilder_jump(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, uint16_t targetLabel)
{
    uint16_t currentPosition = beacon_ByteArrayList_size(methodBuilder->bytecodes);
    int16_t delta = targetLabel - currentPosition;
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x10 | BeaconBytecodeJump);
    beacon_ByteArrayList_addInt16(context, methodBuilder->bytecodes, beacon_BytecodeValue_encode(delta, BytecodeArgumentTypeJumpDelta));
}

void beacon_BytecodeCodeBuilder_jumpIfTrue(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t condition, uint16_t targetLabel)
{
    uint16_t currentPosition = beacon_ByteArrayList_size(methodBuilder->bytecodes);
    int16_t delta = targetLabel - currentPosition;
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x20 | BeaconBytecodeJumpIfTrue);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, condition);
    beacon_ByteArrayList_addInt16(context, methodBuilder->bytecodes, beacon_BytecodeValue_encode(delta, BytecodeArgumentTypeJumpDelta));
}

void beacon_BytecodeCodeBuilder_jumpIfFalse(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t condition, uint16_t targetLabel)
{
    uint16_t currentPosition = beacon_ByteArrayList_size(methodBuilder->bytecodes);
    int16_t delta = targetLabel - currentPosition;
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x20 | BeaconBytecodeJumpIfTrue);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, condition);
    beacon_ByteArrayList_addInt16(context, methodBuilder->bytecodes, beacon_BytecodeValue_encode(delta, BytecodeArgumentTypeJumpDelta));
}

uint8_t beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, size_t argumentCount)
{
    assert(argumentCount <= 0xFF);
    if(argumentCount <= 0xF)
        return (argumentCount << 4);

    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, (0xF0 & argumentCount) | BeaconBytecodeExtendArguments);
    return (argumentCount & 0xF) << 4;
}

void beacon_BytecodeCodeBuilder_sendMessage(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t receiver, beacon_BytecodeValue_t selector, size_t argumentCount, beacon_BytecodeValue_t *arguments)
{
    uint8_t argumentCountBits = beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(context, methodBuilder, 2 + argumentCount);
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, argumentCountBits | BeaconBytecodeSendMessage);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, selector);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, receiver);
    for(size_t i = 0; i < argumentCount; ++i)
        beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, arguments[i]);
}

void beacon_BytecodeCodeBuilder_superSendMessage(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t receiver, beacon_BytecodeValue_t selector, size_t argumentCount, beacon_BytecodeValue_t *arguments)
{
    uint8_t argumentCountBits = beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(context, methodBuilder, 2 + argumentCount);
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, argumentCountBits | BeaconBytecodeSuperSendMessage);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, selector);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, receiver);
    for(size_t i = 0; i < argumentCount; ++i)
        beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, arguments[i]);
}

void beacon_BytecodeCodeBuilder_localReturn(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultValue)
{
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x10 | BeaconBytecodeLocalReturn);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, resultValue);
}

void beacon_BytecodeCodeBuilder_nonLocalReturn(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultValue)
{
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x10 | BeaconBytecodeNonLocalReturn);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, resultValue);
}

void beacon_BytecodeCodeBuilder_makeArray(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, size_t elementCount, beacon_BytecodeValue_t *arguments)
{
    uint8_t argumentCountBits = beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(context, methodBuilder, elementCount);
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, argumentCountBits | BeaconBytecodeMakeArray);
    for(size_t i = 0; i < elementCount; ++i)
        beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, arguments[i]);
}

void beacon_BytecodeCodeBuilder_makeClosureInstance(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t closure, size_t captureCount, beacon_BytecodeValue_t *captures)
{
    uint8_t argumentCountBits = beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(context, methodBuilder, 1 + captureCount);
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, argumentCountBits | BeaconBytecodeMakeArray);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, closure);
    for(size_t i = 0; i < captureCount; ++i)
        beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, captures[i]);
}

beacon_oop_t beacon_interpretBytecodeMethod(beacon_context_t *context, beacon_CompiledCode_t *method, beacon_oop_t receiver, beacon_oop_t selector, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_BytecodeCode_t *code = method->bytecodeImplementation;
    assert(beacon_decodeSmallInteger(code->argumentCount) == (intptr_t)argumentCount);
    intptr_t temporaryCount = beacon_decodeSmallInteger(code->temporaryCount);
    assert(temporaryCount >= 0);

    uint32_t pc = 0;
    uint8_t *bytecodes = code->bytecodes->elements;
    size_t bytecodesSize = code->bytecodes->super.super.super.super.super.header.slotCount;
    uint8_t extendedArgumentCount = 0;

    while(pc < bytecodesSize)
    {
        uint32_t instructionPC = pc;

        abort();
    }
    abort();
}
