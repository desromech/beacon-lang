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
