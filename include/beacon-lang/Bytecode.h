#ifndef BEACON_LANG_BYTECODE_H
#define BEACON_LANG_BYTECODE_H

#include "ObjectModel.h"

typedef enum beacon_BytecodeOpcode_e
{
    BeaconBytecodeNop = 0,
    BeaconBytecodeJump,
    BeaconBytecodeJumpIfTrue,
    BeaconBytecodeJumpIfFalse,
    BeaconBytecodeSendMessage,
    BeaconBytecodeSuperSendMessage,
    BeaconBytecodeLocalReturn,
    BeaconBytecodeNonLocalReturn,
    BeaconBytecodeMakeArray,
    BeaconBytecodeMakeClosureInstance,
    BeaconBytecodeExtendArguments,
} beacon_BytecodeOpcode_t;

typedef enum beacon_BytecodeArgumentType_e
{
    BytecodeArgumentTypeArgument = 0,
    BytecodeArgumentTypeLiteral,
    BytecodeArgumentTypeTemporary,
    BytecodeArgumentTypeJumpDelta,
    BytecodeArgumentTypeCapture
} beacon_BytecodeValueType_t;

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

beacon_BytecodeCodeBuilder_t *beacon_BytecodeCodeBuilder_new(beacon_context_t *context);

/**
 * Finishes the construction of a bytecode method.
 */
beacon_BytecodeCode_t *beacon_BytecodeCodeBuilder_finish(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *builder);

// Bytecode assembly label.
uint16_t beacon_BytecodeCodeBuilder_label(beacon_BytecodeCodeBuilder_t *methodBuilder);


/**
 * No operation.
 */
void beacon_BytecodeCodeBuilder_nop(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder);

/**
 * Unconditional branch.
 */
void beacon_BytecodeCodeBuilder_jump(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, uint16_t targetLabel);

/**
 * Jump ifTrue
 */
void beacon_BytecodeCodeBuilder_jumpIfTrue(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t condition, uint16_t targetLabel);

/**
 * Jump ifFalse
 */
void beacon_BytecodeCodeBuilder_jumpIfFalse(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t condition, uint16_t targetLabel);

/**
 * Send a message.
 */
void beacon_BytecodeCodeBuilder_sendMessage(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t receiver, beacon_BytecodeValue_t selector, size_t argumentCount, beacon_BytecodeValue_t *arguments);

/**
 * Send a message to the superclass
 */
void beacon_BytecodeCodeBuilder_superSendMessage(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t receiver, beacon_BytecodeValue_t selector, size_t argumentCount, beacon_BytecodeValue_t *arguments);

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
void beacon_BytecodeCodeBuilder_makeArray(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, size_t elementCount, beacon_BytecodeValue_t *arguments);

/**
 * Makes a closure instance
 */
void beacon_BytecodeCodeBuilder_makeClosureInstance(beacon_context_t *context, beacon_BytecodeCodeBuilder_t *methodBuilder, beacon_BytecodeValue_t closure, size_t captureCount, beacon_BytecodeValue_t *captures);

#endif //BEACON_LANG_BYTECODE_H
