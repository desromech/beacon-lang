#ifndef BEACON_LANG_BYTECODE_H
#define BEACON_LANG_BYTECODE_H

#include <stdint.h>

typedef enum beacon_BytecodeOpcode_e
{
    BeaconBytecodeNop = 0,
    BeaconBytecodeJumpIfTrue,
    BeaconBytecodeJumpIfFalse,
    BeaconBytecodeSendMessage,
    BeaconBytecodeLocalReturn,
    BeaconBytecodeNonLocalReturn,
    BeaconBytecodeMakeArray,
    BeaconBytecodeMakeClosure,
    BeaconBytecodeExtendArguments,
} beacon_BytecodeOpcode_t

static inline beacon_BytecodeOpcode_t beacon_getBytecodeOpcode(uint8_t bytecode)
{
    return bytecode & 0xF;
}

static inline beacon_BytecodeOpcode_t beacon_getBytecodeArgumentCount(uint8_t bytecode)
{
    return (bytecode >> 4) & 0xF;
}

#endif //BEACON_LANG_BYTECODE_H
