#include "beacon-lang/Bytecode.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/ArrayList.h"
#include "beacon-lang/Exceptions.h"
#include <stdlib.h>
#include <stdio.h>

static inline bool beacon_bytecodeWritesToTemporary(beacon_BytecodeOpcode_t opcode)
{
    switch(opcode)
    {
    case BeaconBytecodeSendMessage:
    case BeaconBytecodeSuperSendMessage:
    case BeaconBytecodeStoreValue:
    case BeaconBytecodeMakeArray:
    case BeaconBytecodeMakeClosureInstance:
        return true;
    default:
        return false;
    }
}

beacon_BytecodeCodeBuilder_t *beacon_BytecodeCodeBuilder_new(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *parentBuilder)
{
    beacon_BytecodeCodeBuilder_t *builder = beacon_allocateObjectWithBehavior(context->heap, context->classes.bytecodeCodeBuilderClass, sizeof(beacon_BytecodeCodeBuilder_t), BeaconObjectKindPointers);
    builder->arguments = beacon_ArrayList_new(context);
    builder->temporaries = beacon_ArrayList_new(context);
    builder->literals = beacon_ArrayList_new(context);
    builder->captures = beacon_ArrayList_new(context);
    builder->bytecodes = beacon_ByteArrayList_new(context);
    builder->parentBuilder = (beacon_oop_t)parentBuilder;
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
            return beacon_BytecodeValue_encode(i, BytecodeArgumentTypeLiteral);

    }

    beacon_ArrayList_add(context, codeBuilder->literals, literal);
    return beacon_BytecodeValue_encode(beacon_ArrayList_size(codeBuilder->literals), BytecodeArgumentTypeLiteral);
}

beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_newTemporary(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, beacon_oop_t optionalNameSymbol)
{
    beacon_ArrayList_add(context, codeBuilder->temporaries, optionalNameSymbol);
    return beacon_BytecodeValue_encode(beacon_ArrayList_size(codeBuilder->temporaries), BytecodeArgumentTypeTemporary);
}

beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_newCapture(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder)
{
    beacon_ArrayList_add(context, codeBuilder->captures, 0);
    return beacon_BytecodeValue_encode(beacon_ArrayList_size(codeBuilder->captures), BytecodeArgumentTypeCapture);
}

beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_getOrCreateSelf(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder)
{
    (void)context;
    (void)codeBuilder;
    return beacon_BytecodeValue_encode(0, BytecodeArgumentTypeArgument);
}

beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_newArgument(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, beacon_oop_t optionalNameSymbol)
{
    beacon_ArrayList_add(context, codeBuilder->arguments, optionalNameSymbol);
    return beacon_BytecodeValue_encode(beacon_ArrayList_size(codeBuilder->arguments), BytecodeArgumentTypeArgument);
}

beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_getReceiverSlot(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, intptr_t index)
{
    (void)codeBuilder;
    BeaconAssert(context, 1<=index);
    return beacon_BytecodeValue_encode(index, BytecodeArgumentTypeReceiverSlot);
}

uint16_t beacon_BytecodeCodeBuilder_label(beacon_BytecodeCodeBuilder_t *methodBuilder)
{
    return beacon_ByteArrayList_size(methodBuilder->bytecodes);
}

void beacon_BytecodeCodeBuilder_nop(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder)
{
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, BeaconBytecodeNop);
}

uint16_t beacon_BytecodeCodeBuilder_jump(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, uint16_t targetLabel)
{
    uint16_t currentPosition = beacon_ByteArrayList_size(methodBuilder->bytecodes);
    int16_t delta = targetLabel - currentPosition;
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x10 | BeaconBytecodeJump);
    beacon_ByteArrayList_addInt16(context, methodBuilder->bytecodes, beacon_BytecodeValue_encode(delta, BytecodeArgumentTypeJumpDelta));
    return currentPosition;
}

void beacon_BytecodeCodeBuilder_fixup_jump(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, uint16_t branchLabel, uint16_t newTarget)
{
    int16_t delta = beacon_BytecodeValue_encode(newTarget - branchLabel, BytecodeArgumentTypeJumpDelta);
    uint8_t deltaLow = delta & 0xFF;
    uint8_t deltaHigh = (delta >> 8) & 0xFF;

    beacon_ByteArrayList_atPut(context, methodBuilder->bytecodes, branchLabel + 2, deltaLow);
    beacon_ByteArrayList_atPut(context, methodBuilder->bytecodes, branchLabel + 3, deltaHigh);
}

uint16_t beacon_BytecodeCodeBuilder_jumpIfTrue(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t condition, uint16_t targetLabel)
{
    uint16_t currentPosition = beacon_ByteArrayList_size(methodBuilder->bytecodes);
    int16_t delta = targetLabel - currentPosition;
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x20 | BeaconBytecodeJumpIfTrue);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, condition);
    beacon_ByteArrayList_addInt16(context, methodBuilder->bytecodes, beacon_BytecodeValue_encode(delta, BytecodeArgumentTypeJumpDelta));
    return currentPosition;
}

uint16_t beacon_BytecodeCodeBuilder_jumpIfFalse(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t condition, uint16_t targetLabel)
{
    uint16_t currentPosition = beacon_ByteArrayList_size(methodBuilder->bytecodes);
    int16_t delta = targetLabel - currentPosition;
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x20 | BeaconBytecodeJumpIfFalse);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, condition);
    beacon_ByteArrayList_addInt16(context, methodBuilder->bytecodes, beacon_BytecodeValue_encode(delta, BytecodeArgumentTypeJumpDelta));
    return currentPosition;
}

void beacon_BytecodeCodeBuilder_fixup_jumpIf(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, uint16_t branchLabel, uint16_t newTarget)
{
    int16_t delta = beacon_BytecodeValue_encode(newTarget - branchLabel, BytecodeArgumentTypeJumpDelta);
    uint8_t deltaLow = delta & 0xFF;
    uint8_t deltaHigh = (delta >> 8) & 0xFF;

    beacon_ByteArrayList_atPut(context, methodBuilder->bytecodes, branchLabel + 4, deltaLow);
    beacon_ByteArrayList_atPut(context, methodBuilder->bytecodes, branchLabel + 5, deltaHigh);
}

uint8_t beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, size_t argumentCount)
{
    BeaconAssert(context, argumentCount <= 0xFF);
    if(argumentCount <= 0xF)
        return (argumentCount << 4);

    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, (0xF0 & argumentCount) | BeaconBytecodeExtendArguments);
    return (argumentCount & 0xF) << 4;
}

void beacon_BytecodeCodeBuilder_sendMessage(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, beacon_BytecodeValue_t receiver, beacon_BytecodeValue_t selector, size_t argumentCount, beacon_BytecodeValue_t *arguments)
{
    uint8_t argumentCountBits = beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(context, methodBuilder, 2 + argumentCount);
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, argumentCountBits | BeaconBytecodeSendMessage);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, resultTemporary);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, receiver);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, selector);
    for(size_t i = 0; i < argumentCount; ++i)
        beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, arguments[i]);
}

void beacon_BytecodeCodeBuilder_superSendMessage(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, beacon_BytecodeValue_t receiver, beacon_BytecodeValue_t selector, size_t argumentCount, beacon_BytecodeValue_t *arguments)
{
    uint8_t argumentCountBits = beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(context, methodBuilder, 2 + argumentCount);
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, argumentCountBits | BeaconBytecodeSuperSendMessage);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, resultTemporary);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, receiver);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, selector);
    for(size_t i = 0; i < argumentCount; ++i)
        beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, arguments[i]);
}

void beacon_BytecodeCodeBuilder_storeValue(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, beacon_BytecodeValue_t valueToStore)
{
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, 0x10 | BeaconBytecodeStoreValue);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, resultTemporary);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, valueToStore);
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

void beacon_BytecodeCodeBuilder_makeArray(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, size_t elementCount, beacon_BytecodeValue_t *arguments)
{
    uint8_t argumentCountBits = beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(context, methodBuilder, elementCount);
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, argumentCountBits | BeaconBytecodeMakeArray);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, resultTemporary);
    for(size_t i = 0; i < elementCount; ++i)
        beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, arguments[i]);
}

void beacon_BytecodeCodeBuilder_makeClosureInstance(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, beacon_BytecodeValue_t closure, size_t captureCount, beacon_BytecodeValue_t *captures)
{
    uint8_t argumentCountBits = beacon_BytecodeCodeBuilder_extendArgumentsIfNeeded(context, methodBuilder, 1 + captureCount);
    beacon_ByteArrayList_add(context, methodBuilder->bytecodes, argumentCountBits | BeaconBytecodeMakeClosureInstance);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, resultTemporary);
    beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, closure);
    for(size_t i = 0; i < captureCount; ++i)
        beacon_ByteArrayList_addUInt16(context, methodBuilder->bytecodes, captures[i]);
}

beacon_oop_t beacon_interpretBytecodeMethod(beacon_context_t *context, beacon_CompiledCode_t *method, beacon_oop_t receiver, beacon_oop_t selector, beacon_oop_t captures, size_t argumentCount, beacon_oop_t *arguments)
{
    (void)selector;
    (void)captures;
    beacon_BytecodeCode_t *code = method->bytecodeImplementation;
    BeaconAssert(context, beacon_decodeSmallInteger(code->argumentCount) == (intptr_t)argumentCount);
    intptr_t temporaryCount = beacon_decodeSmallInteger(code->temporaryCount);
    BeaconAssert(context, temporaryCount >= 0);

    uint32_t pc = 0;
    uint8_t *bytecodes = code->bytecodes->elements;
    size_t bytecodesSize = code->bytecodes->super.super.super.super.super.header.slotCount;
    uint8_t extendedArgumentCount = 0;
    size_t receiverSlotCount = 0;
    beacon_oop_t *receiverSlots = NULL;

    if(receiver)
    {
        receiverSlotCount = ((beacon_ObjectHeader_t*)receiver)->slotCount;
        receiverSlots = (beacon_oop_t*)((beacon_ObjectHeader_t*)receiver + 1);
    }

    beacon_Array_t *capturesArray = (beacon_Array_t*)captures;
    size_t captureCount = capturesArray ? capturesArray->super.super.super.super.super.header.slotCount : 0;

    beacon_oop_t bytecodeDecodedArguments[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS];
    memset(bytecodeDecodedArguments, 0, sizeof(bytecodeDecodedArguments));
    beacon_oop_t temporaryStorage[temporaryCount];
    memset(temporaryStorage, 0, sizeof(temporaryStorage));

    beacon_StackFrameRecord_t stackFrameRecord = {
        .kind = StackFrameBytecodeMethodRecord,
        .context = context,
        .previousRecord = NULL,
        .bytecodeMethodStackRecord = {
            .code = method,
            .receiver = receiver,
            .argumentCount = argumentCount,
            .arguments = arguments,
            .temporaryCount = temporaryCount,
            .temporaries = temporaryStorage,
            .decodedArgumentsTemporaryZoneSize = BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS,
            .decodedArgumentsTemporaryZone = bytecodeDecodedArguments,
            .captures = captures,
        }
    };

    if(setjmp(stackFrameRecord.bytecodeMethodStackRecord.nonLocalReturnJumpBuffer))
    {
        beacon_popStackFrameRecord(&stackFrameRecord);
        return stackFrameRecord.bytecodeMethodStackRecord.returnResultValue;
    }

    beacon_pushStackFrameRecord(&stackFrameRecord);
    
    while(pc < bytecodesSize)
    {
        uint32_t instructionPC = pc;
        int16_t branchDestinationDelta = 0;
        uint32_t branchDestinationPC = instructionPC;
        uint8_t instruction = bytecodes[pc++];

        uint8_t instructionArgumentCount = (extendedArgumentCount << 4) | beacon_getBytecodeArgumentCount(instruction);
        BeaconAssert(context, instructionArgumentCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
        beacon_BytecodeOpcode_t opcode = beacon_getBytecodeOpcode(instruction);

        // Special opcode.
        if(opcode == BeaconBytecodeExtendArguments)
        {
            extendedArgumentCount = instructionArgumentCount;
            continue;
        }

        // Decode the result destination.
        bool writesToTemporary = beacon_bytecodeWritesToTemporary(opcode);
        size_t resultTemporaryOrInstanceVarIndex = 0;
        bool resultTemporaryIsReceiverSlot = false;
        if(writesToTemporary)
        {
            beacon_BytecodeValue_t bytecodeResultTemporary = bytecodes[pc++];
            bytecodeResultTemporary |= (bytecodes[pc++]) << 8;
            BeaconAssert(context, beacon_BytecodeValue_getType(bytecodeResultTemporary) == BytecodeArgumentTypeTemporary ||
                                  beacon_BytecodeValue_getType(bytecodeResultTemporary) == BytecodeArgumentTypeReceiverSlot);
            resultTemporaryOrInstanceVarIndex = beacon_BytecodeValue_getIndex(bytecodeResultTemporary);
            resultTemporaryIsReceiverSlot = beacon_BytecodeValue_getType(bytecodeResultTemporary) == BytecodeArgumentTypeReceiverSlot;
        }

        // Fetch all of the instruction arguments.
        for(uint8_t i = 0; i < instructionArgumentCount; ++i)
        {
            beacon_BytecodeValue_t bytecodeArgument = bytecodes[pc++];
            bytecodeArgument |= (bytecodes[pc++]) << 8;

            uint16_t bytecodeArgumentIndex = beacon_BytecodeValue_getIndex(bytecodeArgument);
            int16_t bytecodeArgumentSignedIndex = beacon_BytecodeValue_getSignedIndex(bytecodeArgument);
            beacon_oop_t *currentDecodedArgument = bytecodeDecodedArguments + i;

            switch(beacon_BytecodeValue_getType(bytecodeArgument))
            {
            case BytecodeArgumentTypeArgument:
                BeaconAssert(context, bytecodeArgumentIndex <= argumentCount);
                if(bytecodeArgumentIndex == 0)
                    *currentDecodedArgument = receiver;
                else
                    *currentDecodedArgument = arguments[bytecodeArgumentIndex - 1];
                break;
            case BytecodeArgumentTypeLiteral:
                BeaconAssert(context, bytecodeArgumentIndex <= code->literals->super.super.super.super.super.header.slotCount);
                *currentDecodedArgument = bytecodeArgumentIndex == 0 ? 0 : code->literals->elements[bytecodeArgumentIndex - 1];
                break;
            case BytecodeArgumentTypeTemporary:
                BeaconAssert(context, bytecodeArgumentIndex <= temporaryCount);
                *currentDecodedArgument = bytecodeArgumentIndex == 0 ? 0 : temporaryStorage[bytecodeArgumentIndex - 1];
                break;
            case BytecodeArgumentTypeJumpDelta:
                branchDestinationDelta = bytecodeArgumentSignedIndex;
                branchDestinationPC += branchDestinationDelta;
                break;
            case BytecodeArgumentTypeCapture:
                BeaconAssert(context, 0 < bytecodeArgumentIndex && bytecodeArgumentIndex <= captureCount);
                *currentDecodedArgument = capturesArray->elements[bytecodeArgumentIndex - 1];
                break;
            case BytecodeArgumentTypeReceiverSlot:
                BeaconAssert(context, 0 < bytecodeArgumentIndex && bytecodeArgumentIndex <= receiverSlotCount);
                *currentDecodedArgument = receiverSlots[bytecodeArgumentIndex - 1];
                break;
            default:
                beacon_exception_error(context, "Invalid bytecode value type");
                break;
            }
        }

        // Execute the instruction.
        beacon_oop_t instructionExecutionResult = 0;
        switch(opcode)
        {
        case BeaconBytecodeNop:
            // Nothing is required here.
            break;
        case BeaconBytecodeJump:
            pc = branchDestinationPC;
            if(branchDestinationDelta < 0)
                beacon_memoryHeapSafepoint(context);
            break;
        case BeaconBytecodeJumpIfTrue:
            if(bytecodeDecodedArguments[0] == context->roots.trueValue)
            {
                pc = branchDestinationPC;
                if(branchDestinationDelta < 0)
                    beacon_memoryHeapSafepoint(context);
            }
            break;
        case BeaconBytecodeJumpIfFalse:
            if(bytecodeDecodedArguments[0] == context->roots.falseValue)
            {
                pc = branchDestinationPC;
                if(branchDestinationDelta < 0)
                    beacon_memoryHeapSafepoint(context);
            }
            break;
        case BeaconBytecodeSendMessage:
            BeaconAssert(context, writesToTemporary);
            instructionExecutionResult = beacon_performWithArguments(context, bytecodeDecodedArguments[0], bytecodeDecodedArguments[1], instructionArgumentCount - 2, bytecodeDecodedArguments + 2);
            break;
        case BeaconBytecodeStoreValue:
            BeaconAssert(context, writesToTemporary);
            instructionExecutionResult = bytecodeDecodedArguments[0];
            break;
        case BeaconBytecodeLocalReturn:
            BeaconAssert(context, instructionArgumentCount == 1);
            stackFrameRecord.bytecodeMethodStackRecord.returnResultValue = bytecodeDecodedArguments[0];
            beacon_popStackFrameRecord(&stackFrameRecord);
            return stackFrameRecord.bytecodeMethodStackRecord.returnResultValue;
        case BeaconBytecodeMakeArray:
            {
                BeaconAssert(context, writesToTemporary);
                beacon_Array_t *resultArray = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + instructionArgumentCount*sizeof(beacon_oop_t), BeaconObjectKindPointers);
                for(size_t i = 0; i < instructionArgumentCount; ++i)
                    resultArray->elements[i] = bytecodeDecodedArguments[i];
                instructionExecutionResult = (beacon_oop_t)resultArray;
            }
            break;
        case BeaconBytecodeMakeClosureInstance:
            {
                BeaconAssert(context, writesToTemporary);

                beacon_BlockClosure_t *blockClosure = beacon_allocateObjectWithBehavior(context->heap, context->classes.blockClosureClass, sizeof(beacon_BlockClosure_t), BeaconObjectKindPointers);
                blockClosure->code = (beacon_CompiledBlock_t*)bytecodeDecodedArguments[0];

                beacon_Array_t *captures = beacon_allocateObjectWithBehavior(context->heap, context->classes.arrayClass, sizeof(beacon_Array_t) + (instructionArgumentCount - 1)*sizeof(beacon_oop_t), BeaconObjectKindPointers);
                blockClosure->captures = (beacon_oop_t)captures;
                for(size_t i = 1; i < instructionArgumentCount; ++i)
                    captures->elements[i - 1] = bytecodeDecodedArguments[i];

                instructionExecutionResult = (beacon_oop_t)blockClosure;
            }
            break;
        default:
            {
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "Unsupported bytecode with opcode %x.\n", opcode);
                beacon_exception_error(context, buffer);
            }
            break;
        }

        // Write back the result.
        if(writesToTemporary && resultTemporaryOrInstanceVarIndex > 0)
        {
            if(resultTemporaryIsReceiverSlot)
                receiverSlots[resultTemporaryOrInstanceVarIndex - 1] = instructionExecutionResult;
            else
                temporaryStorage[resultTemporaryOrInstanceVarIndex - 1] = instructionExecutionResult;
        }
    }

    return 0;
}
