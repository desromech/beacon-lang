#ifndef BEACON_LANG_BYTECODE_H
#define BEACON_LANG_BYTECODE_H

#pragma once 

#include "ObjectModel.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum beacon_BytecodeOpcode_e
{
    BeaconBytecodeNop = 0,
    BeaconBytecodeJump,
    BeaconBytecodeJumpIfTrue,
    BeaconBytecodeJumpIfFalse,
    BeaconBytecodeLocalReturn,
    BeaconBytecodeNonLocalReturn,
    BeaconBytecodeSendMessage,
    BeaconBytecodeSuperSendMessage,
    BeaconBytecodeStoreValue,
    BeaconBytecodeMakeArray,
    BeaconBytecodeMakeClosureInstance,
    BeaconBytecodeExtendArguments,
} beacon_BytecodeOpcode_t;

typedef enum beacon_BytecodeArgumentType_e
{
    BytecodeArgumentTypeLiteral = 0,
    BytecodeArgumentTypeArgument,
    BytecodeArgumentTypeTemporary,
    BytecodeArgumentTypeJumpDelta,
    BytecodeArgumentTypeCapture,
    BytecodeArgumentTypeReceiverSlot,
    BytecodeArgumentTypeSuperReceiver
} beacon_BytecodeValueType_t;

#define BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS 32

typedef uint16_t beacon_BytecodeValue_t;

static inline beacon_BytecodeOpcode_t beacon_getBytecodeOpcode(uint8_t bytecode)
{
    return bytecode & 0xF;
}

static inline beacon_BytecodeOpcode_t beacon_getBytecodeArgumentCount(uint8_t bytecode)
{
    return (bytecode >> 4) & 0xF;
}

static inline beacon_BytecodeValueType_t beacon_BytecodeValue_getType(beacon_BytecodeValue_t value)
{
    return (beacon_BytecodeValueType_t)(value & 7);
}

static inline uint16_t beacon_BytecodeValue_getIndex(beacon_BytecodeValue_t value)
{
    return value >> 3;
}

static inline int16_t beacon_BytecodeValue_getSignedIndex(beacon_BytecodeValue_t value)
{
    return (int16_t)value >> 3;
}

static inline int16_t beacon_BytecodeValue_encode(uint16_t index, beacon_BytecodeValueType_t type)
{
    return (index << 3) | type;
}

beacon_BytecodeCodeBuilder_t *beacon_BytecodeCodeBuilder_new(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *parentBuilder);

/**
 * Finishes the construction of a bytecode method.
 */
beacon_BytecodeCode_t *beacon_BytecodeCodeBuilder_finish(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *builder);

// Add a literal value.
beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_addLiteral(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, beacon_oop_t literal);

// Super receiver class.
beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_superReceiverClass(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, beacon_oop_t literalBehavior);

// New temporary
beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_newTemporary(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, beacon_oop_t optionalNameSymbol);

// New capture
beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_newCapture(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder);

// The receiver value
beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_getOrCreateSelf(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder);

// New argument
beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_newArgument(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, beacon_oop_t optionalNameSymbol);

// Receiver slot
beacon_BytecodeValue_t beacon_BytecodeCodeBuilder_getReceiverSlot(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *codeBuilder, intptr_t index);

// Bytecode assembly label.
uint16_t beacon_BytecodeCodeBuilder_label(beacon_BytecodeCodeBuilder_t *methodBuilder);

/**
 * No operation.
 */
void beacon_BytecodeCodeBuilder_nop(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder);

/**
 * Unconditional branch.
 */
uint16_t beacon_BytecodeCodeBuilder_jump(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, uint16_t targetLabel);

/**
 * Fixup branch.
 */
void beacon_BytecodeCodeBuilder_fixup_jump(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, uint16_t branchLabel, uint16_t newTarget);


/**
 * Jump ifTrue
 */
uint16_t beacon_BytecodeCodeBuilder_jumpIfTrue(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t condition, uint16_t targetLabel);

/**
 * Jump ifFalse
 */
uint16_t beacon_BytecodeCodeBuilder_jumpIfFalse(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t condition, uint16_t targetLabel);

/**
 * Fixup branch if.
 */
void beacon_BytecodeCodeBuilder_fixup_jumpIf(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, uint16_t branchLabel, uint16_t newTarget);

/**
 * Send a message.
 */
void beacon_BytecodeCodeBuilder_sendMessage(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, beacon_BytecodeValue_t receiver, beacon_BytecodeValue_t selector, size_t argumentCount, beacon_BytecodeValue_t *arguments);

/**
 * Send a message to the superclass
 */
void beacon_BytecodeCodeBuilder_superSendMessage(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, beacon_BytecodeValue_t receiverClass, beacon_BytecodeValue_t selector, size_t argumentCount, beacon_BytecodeValue_t *arguments);

/**
 * Stores the given value
 */
void beacon_BytecodeCodeBuilder_storeValue(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, beacon_BytecodeValue_t valueToStore);

/**
 * Local return
 */
void beacon_BytecodeCodeBuilder_localReturn(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultValue);

/**
 * Non-local return
 */
void beacon_BytecodeCodeBuilder_nonLocalReturn(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultValue);

/**
 * Makes an array.
 */
void beacon_BytecodeCodeBuilder_makeArray(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, size_t elementCount, beacon_BytecodeValue_t *arguments);

/**
 * Makes a closure instance
 */
void beacon_BytecodeCodeBuilder_makeClosureInstance(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t resultTemporary, beacon_BytecodeValue_t closure, size_t captureCount, beacon_BytecodeValue_t *captures);

/**
 * Bytecode interpretation.
 */
beacon_oop_t beacon_interpretBytecodeMethod(beacon_context_t *context, beacon_CompiledCode_t *method, beacon_oop_t receiver, beacon_oop_t selector, beacon_oop_t captures, size_t argumentCount, beacon_oop_t *arguments);

#ifdef __cplusplus
}
#endif

#endif //BEACON_LANG_BYTECODE_H
