#include "beacon-lang/BytecodeJit.h"
#include "beacon-lang/Memory.h"
#include <stdlib.h>

#if defined(BEACON_JIT_SUPPORTED) && defined(BEACON_ARCH_X86_64)

typedef enum beacon_x86_register_e
{
#if defined(BEACON_ARCH_X86_64) 
    BEACON_X86_RAX = 0,
    BEACON_X86_RCX = 1,
    BEACON_X86_RDX = 2,
    BEACON_X86_RBX = 3,
    BEACON_X86_RSP = 4,
    BEACON_X86_RBP = 5,
    BEACON_X86_RSI = 6,
    BEACON_X86_RDI = 7,

    BEACON_X86_R8 = 8,
    BEACON_X86_R9 = 9,
    BEACON_X86_R10 = 10,
    BEACON_X86_R11 = 11,
    BEACON_X86_R12 = 12,
    BEACON_X86_R13 = 13,
    BEACON_X86_R14 = 14,
    BEACON_X86_R15 = 15,
#endif

    BEACON_X86_EAX = 0,
    BEACON_X86_ECX = 1,
    BEACON_X86_EDX = 2,
    BEACON_X86_EBX = 3,
    BEACON_X86_ESP = 4,
    BEACON_X86_EBP = 5,
    BEACON_X86_ESI = 6,
    BEACON_X86_EDI = 7,

    BEACON_X86_REG_HALF_MASK = 7,

#if defined(BEACON_ARCH_X86_64)
#ifdef _WIN32
    BEACON_X86_WIN64_ARG0 = BEACON_X86_RCX,
    BEACON_X86_WIN64_ARG1 = BEACON_X86_RDX,
    BEACON_X86_WIN64_ARG2 = BEACON_X86_R8,
    BEACON_X86_WIN64_ARG3 = BEACON_X86_R9,
    BEACON_X86_WIN64_SHADOW_SPACE = 32,

    BEACON_X86_64_ARG0 = BEACON_X86_WIN64_ARG0,
    BEACON_X86_64_ARG1 = BEACON_X86_WIN64_ARG1,
    BEACON_X86_64_ARG2 = BEACON_X86_WIN64_ARG2,
    BEACON_X86_64_ARG3 = BEACON_X86_WIN64_ARG3,
    BEACON_X86_64_CALL_SHADOW_SPACE = BEACON_X86_WIN64_SHADOW_SPACE,
#else
    BEACON_X86_SYSV_ARG0 = BEACON_X86_RDI,
    BEACON_X86_SYSV_ARG1 = BEACON_X86_RSI,
    BEACON_X86_SYSV_ARG2 = BEACON_X86_RDX,
    BEACON_X86_SYSV_ARG3 = BEACON_X86_RCX,
    BEACON_X86_SYSV_ARG4 = BEACON_X86_R8,
    BEACON_X86_SYSV_ARG5 = BEACON_X86_R9,

    BEACON_X86_64_ARG0 = BEACON_X86_SYSV_ARG0,
    BEACON_X86_64_ARG1 = BEACON_X86_SYSV_ARG1,
    BEACON_X86_64_ARG2 = BEACON_X86_SYSV_ARG2,
    BEACON_X86_64_ARG3 = BEACON_X86_SYSV_ARG3,
    BEACON_X86_64_CALL_SHADOW_SPACE = 0,
#endif
#endif
} beacon_x86_register_t;

static void beacon_jit_x86_mov64Absolute(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, uint64_t value);
static void beacon_jit_moveRegisterToOperand(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t operand, beacon_x86_register_t reg);
static void beacon_jit_moveOperandToRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t reg, beacon_bytecodeJitDecodedOperand_t operand);
static void beacon_jit_moveOperandToCallArgumentVector(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t operand, int32_t callArgumentVectorIndex);

static uint8_t beacon_jit_x86_modRM(int8_t rm, uint8_t regOpcode, uint8_t mod)
{
    return (rm & BEACON_X86_REG_HALF_MASK) | ((regOpcode & BEACON_X86_REG_HALF_MASK) << 3) | (mod << 6);
}

static uint8_t beacon_jit_x86_sibOnlyBase(uint8_t reg)
{
    return (reg & BEACON_X86_REG_HALF_MASK) | (4 << 3) ;
}

static uint8_t beacon_jit_x86_modRMRegister(beacon_x86_register_t rm, beacon_x86_register_t reg)
{
    return beacon_jit_x86_modRM(rm, reg, 3);
}

static uint8_t beacon_jit_x86_rex(bool W, bool R, bool X, bool B)
{
    return 0x40 | ((W ? 1 : 0) << 3) | ((R ? 1 : 0) << 2) | ((X ? 1 : 0) << 1) | (B ? 1 : 0);
}

static void beacon_jit_x86_int3(beacon_bytecodeJit_t *jit)
{
    uint8_t instruction[] = {
        0xCC,
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_ud2(beacon_bytecodeJit_t *jit)
{
    uint8_t instruction[] = {
        0x0F, 0x0B,
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_callAbsoluteNon64InlineConstant(beacon_bytecodeJit_t *jit, void *functionPointer)
{
    size_t constantOffset = beacon_bytecodeJit_addConstantsBytes(jit, sizeof(functionPointer), (uint8_t*)&functionPointer);
    uint8_t instruction[] = {
        0xFF,
        beacon_jit_x86_modRM(5, 2, 0),
        0x00, 0x00, 0x00, 0x00,
    };

    size_t relocationOffset = beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction) - 4;

    beacon_bytecodeJitRelocation_t relocation = {
        .offset = relocationOffset,
        .type = BEACON_BYTECODE_JIT_RELOCATION_RELATIVE32,
        .value = constantOffset,
        .addend = -4
    };
    beacon_bytecodeJit_addRelocation(jit, relocation);
}

static void beacon_jit_x86_call(beacon_bytecodeJit_t *jit, void *functionPointer)
{
    beacon_jit_x86_callAbsoluteNon64InlineConstant(jit, functionPointer);
}

static void beacon_jit_x86_pushRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t reg)
{
    if(reg > BEACON_X86_REG_HALF_MASK)
        beacon_bytecodeJit_addByte(jit, beacon_jit_x86_rex(false, false, false, true));
    beacon_bytecodeJit_addByte(jit, 0x50 + (reg & BEACON_X86_REG_HALF_MASK));
}

static void beacon_jit_x86_popRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t reg)
{
    if(reg > BEACON_X86_REG_HALF_MASK)
        beacon_bytecodeJit_addByte(jit, beacon_jit_x86_rex(false, false, false, true));
    beacon_bytecodeJit_addByte(jit, 0x58 + (reg & BEACON_X86_REG_HALF_MASK));
}

static void beacon_jit_x86_endbr64(beacon_bytecodeJit_t *jit)
{
    uint8_t instruction[] = {
        0xF3, 0x0F, 0x1E, 0xFA,
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_ret(beacon_bytecodeJit_t *jit)
{
    beacon_bytecodeJit_addByte(jit, 0xc3);
}

static void beacon_jit_x86_mov64Register(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, beacon_x86_register_t source)
{
    uint8_t instruction[] = {
        beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
        0x8B,
        beacon_jit_x86_modRMRegister(source, destination),
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_mov64Absolute(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, uint64_t value)
{
    uint8_t instruction[] = {
        beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
        0xB8 + (destination & BEACON_X86_REG_HALF_MASK),
        value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF,
        (value >> 32) & 0xFF, (value >> 40) & 0xFF, (value >> 48) & 0xFF, (value >> 56) & 0xFF,
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_subImmediate32(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, int32_t value)
{
    if(value == 0)
        return;

    uint8_t instruction[] = {
        beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
        0x81,
        beacon_jit_x86_modRMRegister(destination, 5),
        value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF,
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_movImmediate32(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, int32_t value)
{
    uint8_t instruction[] = {
        beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
        0xC7,
        beacon_jit_x86_modRMRegister(destination, 0),
        value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF,
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_leaRegisterWithOffset(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, beacon_x86_register_t source, int32_t offset)
{
    if((source & BEACON_X86_REG_HALF_MASK) == BEACON_X86_RSP)
    {
        uint8_t instruction[] = {
            beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
            0x8D,
            beacon_jit_x86_modRM(source, destination, 2),
            beacon_jit_x86_sibOnlyBase(source),
            offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
        };

        beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
    }
    else
    {
        uint8_t instruction[] = {
            beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
            0x8D,
            beacon_jit_x86_modRM(source, destination, 2),
            offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
        };

        beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
    }
}

static void beacon_jit_x86_mov64FromMemoryWithOffset(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, beacon_x86_register_t source, int32_t offset)
{
    if(offset == 0 && source != BEACON_X86_RBP)
    {
        if((source & BEACON_X86_REG_HALF_MASK) == BEACON_X86_RSP)
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
                0x8B,
                beacon_jit_x86_modRM(source, destination, 0),
                beacon_jit_x86_sibOnlyBase(source),
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
        else
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
                0x8B,
                beacon_jit_x86_modRM(source, destination, 0),
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
    }
    else
    {
        if((source & BEACON_X86_REG_HALF_MASK) == BEACON_X86_RSP)
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
                0x8B,
                beacon_jit_x86_modRM(source, destination, 2),
                beacon_jit_x86_sibOnlyBase(source),
                offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
        else
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
                0x8B,
                beacon_jit_x86_modRM(source, destination, 2),
                offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
    }
}

static void beacon_jit_x86_mov64IntoMemoryWithOffset(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, int32_t offset, beacon_x86_register_t source)
{
    if(offset == 0 && (destination & BEACON_X86_REG_HALF_MASK) != BEACON_X86_RBP)
    {
        if((destination & BEACON_X86_REG_HALF_MASK) == BEACON_X86_RSP)
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, source > BEACON_X86_REG_HALF_MASK, false, destination > BEACON_X86_REG_HALF_MASK),
                0x89,
                beacon_jit_x86_sibOnlyBase(destination),
                beacon_jit_x86_modRM(destination, source, 0),
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);            
        }
        else
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, source > BEACON_X86_REG_HALF_MASK, false, destination > BEACON_X86_REG_HALF_MASK),
                0x89,
                beacon_jit_x86_modRM(destination, source, 0),
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);            
        }
    }
    else
    {
        if((destination & BEACON_X86_REG_HALF_MASK) == BEACON_X86_RSP)
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, source > BEACON_X86_REG_HALF_MASK, false, destination > BEACON_X86_REG_HALF_MASK),
                0x89,
                beacon_jit_x86_modRM(destination, source, 2),
                beacon_jit_x86_sibOnlyBase(destination),
                offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
        else
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, source > BEACON_X86_REG_HALF_MASK, false, destination > BEACON_X86_REG_HALF_MASK),
                0x89,
                beacon_jit_x86_modRM(destination, source, 2),
                offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
    }
}

static void beacon_jit_x86_movS32IntoMemoryWithOffset(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, int32_t offset, int32_t immediate)
{
    if(offset == 0 && (destination & BEACON_X86_REG_HALF_MASK) != BEACON_X86_RBP)
    {
        if((destination & BEACON_X86_REG_HALF_MASK) == BEACON_X86_RSP)
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
                0xC7,
                beacon_jit_x86_modRM(destination, 0, 0),
                beacon_jit_x86_sibOnlyBase(destination),
                immediate & 0xFF, (immediate >> 8) & 0xFF, (immediate >> 16) & 0xFF, (immediate >> 24) & 0xFF
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
        else
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
                0xC7,
                beacon_jit_x86_modRM(destination, 0, 0),
                immediate & 0xFF, (immediate >> 8) & 0xFF, (immediate >> 16) & 0xFF, (immediate >> 24) & 0xFF
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
    }
    else
    {
        if((destination & BEACON_X86_REG_HALF_MASK) == BEACON_X86_RSP)
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
                0xC7,
                beacon_jit_x86_modRM(destination, 0, 2),
                beacon_jit_x86_sibOnlyBase(destination),
                offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
                immediate & 0xFF, (immediate >> 8) & 0xFF, (immediate >> 16) & 0xFF, (immediate >> 24) & 0xFF
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
        else
        {
            uint8_t instruction[] = {
                beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
                0xC7,
                beacon_jit_x86_modRM(destination, 0, 2),
                offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
                immediate & 0xFF, (immediate >> 8) & 0xFF, (immediate >> 16) & 0xFF, (immediate >> 24) & 0xFF
            };

            beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
        }
    }
}

static void beacon_jit_x86_logicalShiftRightImmediate(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, uint8_t shiftAmount)
{
    uint8_t instruction[] = {
        beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
        0xC1,
        beacon_jit_x86_modRMRegister(destination, 5),
        shiftAmount
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_mov8IntoMemoryWithOffset(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, int32_t offset, beacon_x86_register_t source)
{
    if(offset == 0 && destination != BEACON_X86_RBP)
    {
        uint8_t instruction[] = {
            beacon_jit_x86_rex(true, source > BEACON_X86_REG_HALF_MASK, false, destination > BEACON_X86_REG_HALF_MASK),
            0x88,
            beacon_jit_x86_modRM(destination, source, 0),
        };

        beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
    }
    else
    {
        uint8_t instruction[] = {
            beacon_jit_x86_rex(true, source > BEACON_X86_REG_HALF_MASK, false, destination > BEACON_X86_REG_HALF_MASK),
            0x88,
            beacon_jit_x86_modRM(destination, source, 2),
            offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
        };

        beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
    }
}

static void beacon_jit_x86_movImmediateI32IntoMemory64WithOffset(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, int32_t offset, int16_t immediate)
{
    uint8_t instruction[] = {
        beacon_jit_x86_rex(true, false, false, destination > BEACON_X86_REG_HALF_MASK),
        0xC7,
        beacon_jit_x86_modRM(destination, 0, 2),
        offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
        immediate & 0xFF, (immediate >> 8) & 0xFF, (immediate >> 16) & 0xFF, (immediate >> 24) & 0xFF,
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}


static void beacon_jit_x86_xorRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, beacon_x86_register_t source)
{
    uint8_t instruction[] = {
        beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
        0x33,
        beacon_jit_x86_modRMRegister(source, destination),
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

static void beacon_jit_x86_jitLoadContextInRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t reg)
{
    beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, BEACON_X86_RBP, jit->contextPointerOffset);
}

void beacon_jit_breakpoint(beacon_bytecodeJit_t *jit)
{
    beacon_jit_x86_int3(jit);
}

void beacon_jit_unreachable(beacon_bytecodeJit_t *jit)
{
    beacon_jit_x86_ud2(jit);
}


static void beacon_jit_moveRegisterToOperand(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t operand, beacon_x86_register_t reg)
{
    int32_t vectorOffset = (int32_t)operand.index * sizeof(void*);
    switch(operand.type)
    {
    case BytecodeArgumentTypeTemporary:
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->localVectorOffset + vectorOffset, reg);
        break;
    default:
        abort();
        break;
    }
}

static void beacon_jit_moveOperandToRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t reg, beacon_bytecodeJitDecodedOperand_t operand)
{
    int32_t vectorOffset = (int32_t)operand.index * sizeof(void*);
    switch(operand.type)
    {
    case BytecodeArgumentTypeArgument:
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, BEACON_X86_RBP, jit->argumentVectorOffset);
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, reg, vectorOffset);
        break;
    case BytecodeArgumentTypeCapture:
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, BEACON_X86_RBP, jit->captureVectorOffset);
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, reg, sizeof(beacon_ObjectHeader_t) + vectorOffset);
        break;
    case BytecodeArgumentTypeLiteral:
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, BEACON_X86_RBP, jit->literalVectorOffset);
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, reg, sizeof(beacon_ObjectHeader_t) + vectorOffset);
        break;
    case BytecodeArgumentTypeTemporary:
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, BEACON_X86_RBP, jit->localVectorOffset + vectorOffset);
        break;
    default:
        abort();
        break;
    }
}

void beacon_jit_moveOperandToOperand(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t destinationOperand, beacon_bytecodeJitDecodedOperand_t sourceOperand)
{
    beacon_jit_moveOperandToRegister(jit, BEACON_X86_RAX, sourceOperand);
    beacon_jit_moveRegisterToOperand(jit, destinationOperand, BEACON_X86_RAX);
}

static void beacon_jit_epilogue(beacon_bytecodeJit_t *jit)
{
#ifdef _WIN32
    beacon_jit_x86_leaRegisterWithOffset(jit, BEACON_X86_RSP, BEACON_X86_RBP, jit->stackFrameSize);
#else
    beacon_jit_x86_mov64Register(jit, BEACON_X86_RSP, BEACON_X86_RBP);
#endif
    beacon_jit_x86_popRegister(jit, BEACON_X86_RBP);
    beacon_jit_x86_ret(jit);
}

void beacon_jit_return(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t operand)
{
    // Disconnect from the stack unwinder.
    beacon_jit_x86_leaRegisterWithOffset(jit, BEACON_X86_64_ARG0, BEACON_X86_RBP, jit->stackFrameRecordOffset);
    beacon_jit_x86_call(jit, &beacon_popStackFrameRecord);

    beacon_jit_moveOperandToRegister(jit, BEACON_X86_RAX, operand);
    beacon_jit_epilogue(jit);
}

void beacon_jit_storePC(beacon_bytecodeJit_t *jit, uint16_t pc)
{
    beacon_jit_x86_movImmediateI32IntoMemory64WithOffset(jit, BEACON_X86_RBP, jit->pcOffset, pc);
}

static void beacon_jit_cfi_beginPrologue(beacon_bytecodeJit_t *jit)
{
    beacon_dwarf_cie_t ehCie = {0};
    ehCie.codeAlignmentFactor = 1;
    ehCie.dataAlignmentFactor = -sizeof(uintptr_t);
    ehCie.pointerSize = sizeof(uintptr_t);
    ehCie.returnAddressRegister = sizeof(uintptr_t) == 8 ? DW_X64_REG_RA : DW_X86_REG_RA;
    jit->dwarfEhBuilder.initialStackFrameSize = 1; // Return address
    jit->dwarfEhBuilder.stackPointerRegister = sizeof(uintptr_t) == 8 ? DW_X64_REG_RSP : DW_X86_REG_ESP;
    beacon_dwarf_cfi_beginCIE(&jit->dwarfEhBuilder, &ehCie);
    beacon_dwarf_cfi_cfaInRegisterWithFactoredOffset(&jit->dwarfEhBuilder, jit->dwarfEhBuilder.stackPointerRegister, 1);
    beacon_dwarf_cfi_registerValueAtFactoredOffset(&jit->dwarfEhBuilder, sizeof(uintptr_t) == 8 ? DW_X64_REG_RA : DW_X86_REG_RA, 1);

    beacon_dwarf_cfi_endCIE(&jit->dwarfEhBuilder);
    beacon_dwarf_cfi_beginFDE(&jit->dwarfEhBuilder, jit->instructions.size);
}

static void beacon_jit_cfi_pushRBP(beacon_bytecodeJit_t *jit)
{
#ifdef _WIN32
    beacon_bytecodeJit_uwop_pushNonVol(jit, 5);
#endif
    beacon_dwarf_cfi_setPC(&jit->dwarfEhBuilder, jit->instructions.size);
    beacon_dwarf_cfi_pushRegister(&jit->dwarfEhBuilder, sizeof(uintptr_t) == 8 ? DW_X64_REG_RBP : DW_X86_REG_EBP);
}

static void beacon_jit_cfi_storeStackInFramePointer(beacon_bytecodeJit_t *jit, int32_t offset)
{
#ifdef _WIN32
    assert((offset % 16) == 0);
    jit->cfiFrameOffset = offset / 16;
    beacon_bytecodeJit_uwop_setFPReg(jit);
#endif
    beacon_dwarf_cfi_setPC(&jit->dwarfEhBuilder, jit->instructions.size);
    beacon_dwarf_cfi_saveFramePointerInRegister(&jit->dwarfEhBuilder, sizeof(uintptr_t) == 8 ? DW_X64_REG_RBP : DW_X86_REG_EBP, offset);
}

static void beacon_jit_cfi_subtract(beacon_bytecodeJit_t *jit, size_t subtractionAmount)
{
    if(!subtractionAmount) return;
#ifdef _WIN32
    beacon_bytecodeJit_uwop_alloc(jit, subtractionAmount);
#endif
    beacon_dwarf_cfi_stackSizeAdvance(&jit->dwarfEhBuilder, jit->instructions.size, subtractionAmount);
}

static void beacon_jit_cfi_endPrologue(beacon_bytecodeJit_t *jit)
{
    jit->prologueSize = jit->instructions.size;
    beacon_dwarf_cfi_endPrologue(&jit->dwarfEhBuilder);
}

void beacon_jit_prologue(beacon_bytecodeJit_t *jit)
{
    beacon_jit_cfi_beginPrologue(jit);
#ifndef _WIN32
    beacon_jit_x86_endbr64(jit);
#endif

    // Allocate the stack storage.
    size_t requiredStackSize = (sizeof(beacon_StackFrameRecord_t)) + jit->localVectorSize * sizeof(intptr_t);
    jit->stackFrameSize = (requiredStackSize + 15) & (-16);
    jit->stackFrameRecordOffset = 0;
    jit->stackCallReservationSize = jit->maxCallArgumentCount *  sizeof(intptr_t);  
    
    //(beacon_context_t *context, beacon_tuple_t function, size_t argumentCount, beacon_tuple_t *arguments)
    beacon_jit_x86_pushRegister(jit, BEACON_X86_RBP);
    beacon_jit_cfi_pushRBP(jit);

#ifdef _WIN32
    jit->stackCallReservationSize += BEACON_X86_64_CALL_SHADOW_SPACE;
    beacon_jit_x86_subImmediate32(jit, BEACON_X86_RSP, jit->stackFrameSize + jit->stackCallReservationSize);
    beacon_jit_cfi_subtract(jit, jit->stackFrameSize + jit->stackCallReservationSize);

    beacon_jit_x86_leaRegisterWithOffset(jit, BEACON_X86_RBP, BEACON_X86_RSP, jit->stackCallReservationSize);
    beacon_jit_cfi_storeStackInFramePointer(jit, jit->stackCallReservationSize);

#else
    beacon_jit_x86_mov64Register(jit, BEACON_X86_RBP, BEACON_X86_RSP);
    beacon_jit_cfi_storeStackInFramePointer(jit, 0);

    beacon_jit_x86_subImmediate32(jit, BEACON_X86_RSP, jit->stackFrameSize + jit->stackCallReservationSize);
    beacon_jit_cfi_subtract(jit, jit->stackFrameSize + jit->stackCallReservationSize);
    jit->stackFrameRecordOffset = -jit->stackFrameSize;
#endif

    beacon_jit_cfi_endPrologue(jit);

    // Build the stack frame record.
    {
        beacon_jit_x86_movS32IntoMemoryWithOffset(jit,
            BEACON_X86_RBP, jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, previousRecord),
            0);

        jit->contextPointerOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, context);
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->contextPointerOffset, BEACON_X86_64_ARG0);
    
        beacon_jit_x86_movS32IntoMemoryWithOffset(jit,
            BEACON_X86_RBP, jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, kind),
            StackFrameBytecodeJitMethodRecord);

        // ReceiverOrCapture
        jit->captureVectorOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, bytecodeJitMethodStackRecord.receiverOrCaptures);
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->captureVectorOffset, BEACON_X86_64_ARG1);
    
        // Literal vector
        beacon_jit_x86_mov64Absolute(jit, BEACON_X86_RAX, (uintptr_t)jit->literalVector); // Pointer to GC root with the literal vector.
        jit->literalVectorOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, bytecodeJitMethodStackRecord.literals);
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->literalVectorOffset, BEACON_X86_RAX);

        // Argument count
        intptr_t argumentCountOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, bytecodeJitMethodStackRecord.argumentCount);
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, argumentCountOffset, BEACON_X86_64_ARG2);

        // Arguments
        jit->argumentVectorOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, bytecodeJitMethodStackRecord.arguments);
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->argumentVectorOffset, BEACON_X86_64_ARG3);
        
        // PC
        jit->pcOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, bytecodeJitMethodStackRecord.pc);
        beacon_jit_x86_movS32IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->pcOffset, 0);

        /*
        jit->callArgumentVectorSizeOffset = jit->stackFrameRecordOffset + offsetof(beacon_stackFrameBytecodeFunctionJitActivationRecord_t, callArgumentVectorSize);
        beacon_jit_x86_movS32IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->callArgumentVectorSizeOffset, 0);

        jit->callArgumentVectorOffset = jit->stackFrameRecordOffset + offsetof(beacon_stackFrameBytecodeFunctionJitActivationRecord_t, callArgumentVector);
        // This is not needed to be cleared.
        */

        size_t localVectorSizeOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, bytecodeJitMethodStackRecord.temporaryCount);
        beacon_jit_x86_movS32IntoMemoryWithOffset(jit, BEACON_X86_RBP, (int32_t)localVectorSizeOffset, (int32_t)jit->localVectorSize);

        jit->localVectorOffset = jit->stackFrameRecordOffset + sizeof(beacon_StackFrameRecord_t);

        // Initialize the locals
        if(jit->localVectorSize > 0)
        {
            beacon_jit_x86_xorRegister(jit, BEACON_X86_RAX, BEACON_X86_RAX);
            for(size_t i = 0; i < jit->localVectorSize; ++i)
            {
                size_t localOffset = jit->localVectorOffset + i*sizeof(void*);
                beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, (int32_t)localOffset, BEACON_X86_RAX);
            }
        }

        // Connect with the stack unwinder.
        beacon_jit_x86_leaRegisterWithOffset(jit, BEACON_X86_64_ARG0, BEACON_X86_RBP, jit->stackFrameRecordOffset);
        beacon_jit_x86_call(jit, &beacon_pushStackFrameRecord);
    }

}


#endif 