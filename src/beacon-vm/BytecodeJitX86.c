#include "beacon-lang/BytecodeJit.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
extern void __register_frame(const void*);
#endif

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

    BEACON_X86_64_SCRATCH_REG0 = BEACON_X86_RAX,
    BEACON_X86_64_SCRATCH_REG1 = BEACON_X86_R10,
    BEACON_X86_64_SCRATCH_REG2 = BEACON_X86_R11,

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

    BEACON_X86_64_SCRATCH_REG0 = BEACON_X86_RAX,
    BEACON_X86_64_SCRATCH_REG1 = BEACON_X86_R10,
    BEACON_X86_64_SCRATCH_REG2 = BEACON_X86_R11,
#endif
#endif
} beacon_x86_register_t;

static void beacon_jit_x86_mov64Absolute(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, uint64_t value);
static void beacon_jit_moveRegisterToOperand(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t operand, beacon_x86_register_t reg);
static void beacon_jit_moveOperandToRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t reg, beacon_bytecodeJitDecodedOperand_t operand);

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
    if(operand.index == 0)
        return;

    int32_t vectorOffset = (int32_t)(operand.index - 1) * sizeof(void*);
    switch(operand.type)
    {
    case BytecodeArgumentTypeTemporary:
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->localVectorOffset + vectorOffset, reg);
        break;
    case BytecodeArgumentTypeReceiverSlot:
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, BEACON_X86_64_SCRATCH_REG2, BEACON_X86_RBP, jit->captureVectorOrReceiverOffset);
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_64_SCRATCH_REG2, sizeof(beacon_ObjectHeader_t) + vectorOffset, reg);
        break;
    default:
        abort();
        break;
    }
}

static void beacon_jit_moveOperandToRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t reg, beacon_bytecodeJitDecodedOperand_t operand)
{
    if(operand.index == 0)
    {
        beacon_jit_x86_xorRegister(jit, reg, reg);
        return;
    }

    int32_t vectorOffset = ((int32_t)operand.index - 1) * sizeof(void*);
    switch(operand.type)
    {
    case BytecodeArgumentTypeArgument:
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, BEACON_X86_RBP, jit->argumentVectorOffset);
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, reg, vectorOffset);
        break;
    case BytecodeArgumentTypeCapture:
    case BytecodeArgumentTypeReceiverSlot:
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, BEACON_X86_RBP, jit->captureVectorOrReceiverOffset);
        beacon_jit_x86_mov64FromMemoryWithOffset(jit, reg, reg, sizeof(beacon_ObjectHeader_t) + vectorOffset);
        break;
    case BytecodeArgumentTypeLiteral:
    case BytecodeArgumentTypeSuperReceiver:
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

void beacon_jit_moveOperandToCallArgumentVector(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t operand, int32_t callArgumentVectorIndex)
{
    beacon_jit_moveOperandToRegister(jit, BEACON_X86_64_SCRATCH_REG0, operand);
    beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RSP, BEACON_X86_64_CALL_SHADOW_SPACE + callArgumentVectorIndex*sizeof(void*), BEACON_X86_64_SCRATCH_REG0);
}

void beacon_jit_moveOperandToOperand(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t destinationOperand, beacon_bytecodeJitDecodedOperand_t sourceOperand)
{
    beacon_jit_moveOperandToRegister(jit, BEACON_X86_64_SCRATCH_REG0, sourceOperand);
    beacon_jit_moveRegisterToOperand(jit, destinationOperand, BEACON_X86_64_SCRATCH_REG0);
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

void beacon_jit_jumpRelative(beacon_bytecodeJit_t *jit, size_t targetPC)
{
    uint8_t instruction[] = {
        0xE9, 0x00, 0x00, 0x00, 0x00,
    };

    size_t relocationOffset = beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction) - 4;
    beacon_bytecodeJitPCRelocation_t relocation = {
        .offset = relocationOffset,
        .targetPC = targetPC,
        .addend = -4,
    };
    beacon_bytecodeJit_addPCRelocation(jit, relocation);
}

static void beacon_jit_x86_cmpRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t destination, beacon_x86_register_t source)
{
    uint8_t instruction[] = {
        beacon_jit_x86_rex(true, destination > BEACON_X86_REG_HALF_MASK, false, source > BEACON_X86_REG_HALF_MASK),
        0x39,
        beacon_jit_x86_modRMRegister(source, destination),
    };

    beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction);
}

void beacon_jit_jumpRelativeIfTrue(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t conditionOperand, size_t targetPC)
{
    beacon_jit_moveOperandToRegister(jit, BEACON_X86_64_SCRATCH_REG0, conditionOperand);
    beacon_jit_x86_mov64Absolute(jit, BEACON_X86_64_SCRATCH_REG1, jit->context->roots.trueValue);
    beacon_jit_x86_cmpRegister(jit, BEACON_X86_64_SCRATCH_REG0, BEACON_X86_64_SCRATCH_REG1);

    uint8_t instruction[] = {
        // Jeq
        0x0F, 0x84, 0x00, 0x00, 0x00, 0x00,
    };

    size_t relocationOffset = beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction) - 4;
    beacon_bytecodeJitPCRelocation_t relocation = {
        .offset = relocationOffset,
        .targetPC = targetPC,
        .addend = -4,
    };
    beacon_bytecodeJit_addPCRelocation(jit, relocation);

}

void beacon_jit_jumpRelativeIfFalse(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t conditionOperand, size_t targetPC)
{
    beacon_jit_moveOperandToRegister(jit, BEACON_X86_64_SCRATCH_REG0, conditionOperand);
    beacon_jit_x86_mov64Absolute(jit, BEACON_X86_64_SCRATCH_REG1, jit->context->roots.falseValue);
    beacon_jit_x86_cmpRegister(jit, BEACON_X86_64_SCRATCH_REG0, BEACON_X86_64_SCRATCH_REG1);

    uint8_t instruction[] = {
        // Jeq
        0x0F, 0x84, 0x00, 0x00, 0x00, 0x00,
    };

    size_t relocationOffset = beacon_bytecodeJit_addBytes(jit, sizeof(instruction), instruction) - 4;
    beacon_bytecodeJitPCRelocation_t relocation = {
        .offset = relocationOffset,
        .targetPC = targetPC,
        .addend = -4,
    };
    beacon_bytecodeJit_addPCRelocation(jit, relocation);
}

void beacon_jit_safepoint(beacon_bytecodeJit_t *jit)
{
    beacon_jit_x86_jitLoadContextInRegister(jit, BEACON_X86_64_ARG0);
    beacon_jit_x86_call(jit, &beacon_memoryHeapSafepoint);
}

void beacon_jit_sendMessage(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t resultOperand, uint32_t totalArgumentCount)
{
    beacon_jit_x86_jitLoadContextInRegister(jit, BEACON_X86_64_ARG0);
    beacon_jit_x86_movImmediate32(jit, BEACON_X86_64_ARG1, totalArgumentCount);
    beacon_jit_x86_leaRegisterWithOffset(jit, BEACON_X86_64_ARG2, BEACON_X86_RSP, BEACON_X86_64_CALL_SHADOW_SPACE);
    beacon_jit_x86_call(jit, &beacon_bytecodeJit_sendMessageTrampoline);
    beacon_jit_moveRegisterToOperand(jit, resultOperand, BEACON_X86_RAX);
}

void beacon_jit_superSendMessage(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t resultOperand, uint32_t totalArgumentCount)
{
    beacon_jit_x86_jitLoadContextInRegister(jit, BEACON_X86_64_ARG0);
    beacon_jit_x86_movImmediate32(jit, BEACON_X86_64_ARG1, totalArgumentCount);
    beacon_jit_x86_leaRegisterWithOffset(jit, BEACON_X86_64_ARG2, BEACON_X86_RSP, BEACON_X86_64_CALL_SHADOW_SPACE);
    beacon_jit_x86_call(jit, &beacon_bytecodeJit_superSendMessageTrampoline);
    beacon_jit_moveRegisterToOperand(jit, resultOperand, BEACON_X86_RAX);
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
        jit->captureVectorOrReceiverOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, bytecodeJitMethodStackRecord.receiverOrCaptures);
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->captureVectorOrReceiverOffset, BEACON_X86_64_ARG1);
    
        // Literal vector
        beacon_jit_x86_mov64Absolute(jit, BEACON_X86_64_SCRATCH_REG0, (uintptr_t)jit->literalVector); // Pointer to GC root with the literal vector.
        jit->literalVectorOffset = jit->stackFrameRecordOffset + offsetof(beacon_StackFrameRecord_t, bytecodeJitMethodStackRecord.literals);
        beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, jit->literalVectorOffset, BEACON_X86_64_SCRATCH_REG0);

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
            beacon_jit_x86_xorRegister(jit, BEACON_X86_64_SCRATCH_REG0, BEACON_X86_64_SCRATCH_REG0);
            for(size_t i = 0; i < jit->localVectorSize; ++i)
            {
                size_t localOffset = jit->localVectorOffset + i*sizeof(void*);
                beacon_jit_x86_mov64IntoMemoryWithOffset(jit, BEACON_X86_RBP, (int32_t)localOffset, BEACON_X86_64_SCRATCH_REG0);
            }
        }

        // Connect with the stack unwinder.
        beacon_jit_x86_leaRegisterWithOffset(jit, BEACON_X86_64_ARG0, BEACON_X86_RBP, jit->stackFrameRecordOffset);
        beacon_jit_x86_call(jit, &beacon_pushStackFrameRecord);
    }

}

static void beacon_jit_emitUnwindInfo(beacon_bytecodeJit_t *jit)
{
#ifdef _WIN32
    RUNTIME_FUNCTION runtimeFunction = {0};
    runtimeFunction.BeginAddress = 0;
    runtimeFunction.EndAddress = (DWORD)jit->instructions.size;
    beacon_bytecodeJit_addUnwindInfoBytes(jit, sizeof(runtimeFunction), (uint8_t*)&runtimeFunction);

    // Unwind_info
    size_t codeCount = jit->unwindInfoBytecode.size/2;
    int frameRegister = /* RBP */ 5;
    int frameOffset = jit->cfiFrameOffset;

    beacon_bytecodeJit_addUnwindInfoByte(jit, /*Version*/1  | (/* Flags*/0 << 3));
    beacon_bytecodeJit_addUnwindInfoByte(jit, (uint8_t)jit->prologueSize);
    beacon_bytecodeJit_addUnwindInfoByte(jit, (uint8_t)codeCount);
    beacon_bytecodeJit_addUnwindInfoByte(jit, (uint8_t) ((frameRegister) | (frameOffset << 4)));

    // Unwind codes must be sorted in descending order.
    uint16_t *unwindCodes = (uint16_t *)jit->unwindInfoBytecode.data;
    for(size_t i = 0; i < codeCount; ++i)
        beacon_DynArray_addAll(&jit->unwindInfo, 2, unwindCodes + codeCount - i - 1);

    if((codeCount % 2) != 0)
    {
        beacon_bytecodeJit_addUnwindInfoByte(jit, 0);
        beacon_bytecodeJit_addUnwindInfoByte(jit, 0);
    }
#endif
    beacon_dwarf_cfi_endFDE(&jit->dwarfEhBuilder, jit->instructions.size);
    beacon_dwarf_cfi_finish(&jit->dwarfEhBuilder);
}

typedef struct beacon_jit_x64_elfSectionHeaders_s
{
    beacon_elf64_sectionHeader_t null;
    beacon_elf64_sectionHeader_t text;
    beacon_elf64_sectionHeader_t eh_frame;
    beacon_elf64_sectionHeader_t debug_line;
    beacon_elf64_sectionHeader_t debug_str;
    beacon_elf64_sectionHeader_t debug_abbrev;
    beacon_elf64_sectionHeader_t debug_info;
    beacon_elf64_sectionHeader_t symtab;
    beacon_elf64_sectionHeader_t str;
    beacon_elf64_sectionHeader_t shstr;
} beacon_jit_x64_elfSectionHeaders_t;

typedef struct beacon_jit_x64_elfSymbolTable_s
{
    beacon_elf64_symbol_t null;
    beacon_elf64_symbol_t sourceFile;
    beacon_elf64_symbol_t text;
    beacon_elf64_symbol_t jittedFunction;
} beacon_jit_x64_elfSymbolTable_t;

typedef struct beacon_jit_x64_elfContentFooter_s
{
    beacon_jit_x64_elfSymbolTable_t symbols;
    beacon_jit_x64_elfSectionHeaders_t sections;
} beacon_jit_x64_elfContentFooter_t;

static size_t beacon_jit_emitObjectFileCString(beacon_bytecodeJit_t *jit, const char *cstring)
{
    size_t result = jit->objectFileContent.size;
    beacon_DynArray_addAll(&jit->objectFileContent, strlen(cstring) + 1, cstring);
    return result;
}

static size_t beacon_jit_emitObjectFileSourceFileName(beacon_bytecodeJit_t *jit)
{
    if(!jit->sourcePosition)
        return 0;

    beacon_SourcePosition_t *sourcePosition = (beacon_SourcePosition_t*)jit->sourcePosition;
    if(!sourcePosition->sourceCode)
        return 0;

    beacon_SourceCode_t *sourceCode = (beacon_SourceCode_t*)sourcePosition->sourceCode;
    
    size_t nameOffset = jit->objectFileContent.size;
    bool hasEmittedName = false;

    if(sourceCode->directory)
    {
        size_t byteSize = sourceCode->directory->super.super.super.super.super.header.slotCount;
        if(byteSize > 0)
        {
            beacon_DynArray_addAll(&jit->objectFileContent, byteSize, sourceCode->directory->data);
            hasEmittedName = true;

            char slash = '/';
            beacon_DynArray_add(&jit->objectFileContent, &slash);
        }
    }

    if(sourceCode->name)
    {
        size_t byteSize = sourceCode->name->super.super.super.super.super.header.slotCount;
        if(byteSize > 0)
        {
            beacon_DynArray_addAll(&jit->objectFileContent, byteSize, sourceCode->name->data);
            hasEmittedName = true;
        }
    }

    if(!hasEmittedName)
        return 0;

    char nullTerminator = 0;
    beacon_DynArray_add(&jit->objectFileContent, &nullTerminator);
    return nameOffset;
}


static size_t beacon_jit_emitObjectFileJittedFunctionName(beacon_bytecodeJit_t *jit)
{
    size_t nameOffset = jit->objectFileContent.size;
    jit->objectFileContentJittedFunctionNameOffset = nameOffset;

    // Emit the program entity name;
    bool hasEmittedName = false;

    // Function source location and pointer number.
    if(!hasEmittedName)
    {
        char pointerBuffer[32];
        uint32_t sourceLine = (uint32_t)beacon_decodeSmallInteger(jit->sourcePosition->startLine);
        uint32_t sourceColumn = (uint32_t)beacon_decodeSmallInteger(jit->sourcePosition->startColumn);

        snprintf(pointerBuffer, sizeof(pointerBuffer), "%d:%d-%08llx", sourceLine, sourceColumn, (unsigned long long)jit->compiledProgramEntity);
        beacon_DynArray_addAll(&jit->objectFileContent, strlen(pointerBuffer), pointerBuffer);
    }

    char nullTerminator = 0;
    beacon_DynArray_add(&jit->objectFileContent, &nullTerminator);

    return nameOffset;
}

static void beacon_jit_emitObjectFile(beacon_bytecodeJit_t *jit)
{
    beacon_elf64_header_t header = {0};
    beacon_jit_x64_elfContentFooter_t footer = {0};

    size_t stringTableOffset = jit->objectFileContent.size;
    footer.sections.null.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ""); // Null string
    footer.sections.text.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".text");
    footer.sections.eh_frame.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".eh_frame");
    footer.sections.debug_line.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".debug_line");
    footer.sections.debug_str.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".debug_str");
    footer.sections.debug_abbrev.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".debug_abbrev");
    footer.sections.debug_info.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".debug_info");
    footer.sections.symtab.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".symtab");
    footer.sections.str.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".str");
    footer.sections.shstr.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileCString(jit, ".shstr");

    footer.symbols.sourceFile.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileSourceFileName(jit);
    footer.symbols.sourceFile.info = BEACON_ELF64_SYM_INFO(BEACON_STT_FILE, BEACON_STB_LOCAL);

    footer.symbols.text.sectionHeaderIndex = 1;
    footer.symbols.text.info = BEACON_ELF64_SYM_INFO(BEACON_STT_SECTION, BEACON_STB_LOCAL);

    footer.symbols.jittedFunction.name = (beacon_elf64_word_t)beacon_jit_emitObjectFileJittedFunctionName(jit);
    footer.symbols.jittedFunction.info = BEACON_ELF64_SYM_INFO(BEACON_STT_FUNC, BEACON_STB_LOCAL);
    footer.symbols.jittedFunction.sectionHeaderIndex = 1;
    footer.symbols.jittedFunction.value = 0;
    footer.symbols.jittedFunction.size = jit->instructions.size;

    header.ident[BEACON_EI_MAG0] = 0x7f;
    header.ident[BEACON_EI_MAG1] = 'E';
    header.ident[BEACON_EI_MAG2] = 'L';
    header.ident[BEACON_EI_MAG3] = 'F';
    header.ident[BEACON_EI_CLASS] = BEACON_ELFCLASS64;
    header.ident[BEACON_EI_DATA] = BEACON_ELFDATA2LSB;
    header.ident[BEACON_EI_VERSION] = BEACON_ELFCURRENT_VERSION;
    header.type = BEACON_ET_REL;
    header.machine = BEACON_EM_X86_64;
    header.elfHeaderSize = sizeof(header);
    header.version = BEACON_ELFCURRENT_VERSION;
    header.sectionHeaderEntrySize = sizeof(footer.sections.null);
    header.sectionHeaderNum = sizeof(footer.sections) / sizeof(footer.sections.null);
    header.sectionHeaderNameStringTableIndex = offsetof(beacon_jit_x64_elfSectionHeaders_t, shstr) / sizeof(beacon_elf64_sectionHeader_t);

    size_t stringTableEnd = jit->objectFileContent.size;
    size_t stringTableSize = stringTableEnd - stringTableOffset;

    footer.sections.text.type = BEACON_SHT_PROGBITS;
    footer.sections.text.flags = BEACON_SHF_ALLOC | BEACON_SHF_EXECINSTR;
    footer.sections.text.addressAlignment = 1;

    footer.sections.eh_frame.type = sizeof(uintptr_t) == 8 ? SHT_X86_64_UNWIND : BEACON_SHT_PROGBITS;
    footer.sections.eh_frame.flags = BEACON_SHF_ALLOC;
    footer.sections.eh_frame.addressAlignment = sizeof(uintptr_t);

    footer.sections.debug_line.type = BEACON_SHT_PROGBITS;
    footer.sections.debug_line.addressAlignment = sizeof(uintptr_t);

    footer.sections.debug_str.type = BEACON_SHT_PROGBITS;
    footer.sections.debug_str.addressAlignment = sizeof(uintptr_t);

    footer.sections.debug_abbrev.type = BEACON_SHT_PROGBITS;
    footer.sections.debug_abbrev.addressAlignment = sizeof(uintptr_t);

    footer.sections.debug_info.type = BEACON_SHT_PROGBITS;
    footer.sections.debug_info.addressAlignment = sizeof(uintptr_t);

    footer.sections.str.type = BEACON_SHT_STRTAB;
    footer.sections.str.offset = stringTableOffset;
    footer.sections.str.addressAlignment = 1;
    footer.sections.str.size = stringTableSize;

    footer.sections.shstr.type = BEACON_SHT_STRTAB;
    footer.sections.shstr.offset = stringTableOffset;
    footer.sections.shstr.addressAlignment = 1;
    footer.sections.shstr.size = stringTableSize;

    size_t symbolTableOffset = jit->objectFileContent.size;
    footer.sections.symtab.type = BEACON_SHT_SYMTAB;
    footer.sections.symtab.offset = symbolTableOffset;
    footer.sections.symtab.entrySize = sizeof(beacon_elf64_symbol_t);
    footer.sections.symtab.addressAlignment = 1;
    footer.sections.symtab.link = offsetof(beacon_jit_x64_elfSectionHeaders_t, str) / sizeof(beacon_elf64_sectionHeader_t);
    footer.sections.symtab.info = sizeof(beacon_jit_x64_elfSymbolTable_t) / sizeof(beacon_elf64_symbol_t);
    footer.sections.symtab.size = sizeof(beacon_jit_x64_elfSymbolTable_t);

    beacon_DynArray_addAll(&jit->objectFileHeader, sizeof(header), &header);
    beacon_DynArray_addAll(&jit->objectFileContent, sizeof(footer), &footer);
}

static void beacon_jit_fixupObjectFile(beacon_bytecodeJit_t *jit,
    beacon_elf64_header_t *header, beacon_elf64_header_t *headerExecutablePointer,
    uint8_t *instructionsExecutablePointer,
    uint8_t *ehFramePointer, uint8_t *ehFrameExecutablePointer,
    uint8_t *debugLineExecutablePointer,
    uint8_t *debugStrExecutablePointer,
    uint8_t *debugAbbrevExecutablePointer,
    uint8_t *debugInfoExecutablePointer,
    uint8_t *objectFileContentExecutablePointer,
    beacon_jit_x64_elfContentFooter_t *footer)
{
    if(jit->dwarfEhBuilder.fdeInitialLocationOffset > 0)
    {
        int32_t *initialLocationPointer = (int32_t*)(ehFramePointer + jit->dwarfEhBuilder.fdeInitialLocationOffset);
        int32_t *initialLocationExecutablePointer = (int32_t*)(ehFrameExecutablePointer + jit->dwarfEhBuilder.fdeInitialLocationOffset);
        *initialLocationPointer = (int32_t) ((uintptr_t)instructionsExecutablePointer - (uintptr_t)initialLocationExecutablePointer);
    }

    header->sectionHeadersOffset = (uintptr_t)&footer->sections - (uintptr_t)header;
    beacon_elf64_off_t contentOffset = (uintptr_t)objectFileContentExecutablePointer - (uintptr_t)headerExecutablePointer;

    footer->sections.text.offset = (uintptr_t)instructionsExecutablePointer - (uintptr_t)headerExecutablePointer;
    footer->sections.text.address = (beacon_elf64_addr_t)instructionsExecutablePointer;
    footer->sections.text.size = jit->instructions.size;

    footer->sections.eh_frame.offset = (uintptr_t)ehFrameExecutablePointer - (uintptr_t)headerExecutablePointer;
    footer->sections.eh_frame.address = (beacon_elf64_addr_t)ehFrameExecutablePointer;
    footer->sections.eh_frame.size = jit->dwarfEhBuilder.buffer.size;

    footer->sections.debug_line.offset = (uintptr_t)debugLineExecutablePointer - (uintptr_t)headerExecutablePointer;
    footer->sections.debug_line.size = jit->dwarfDebugInfoBuilder.line.size;

    footer->sections.debug_str.offset = (uintptr_t)debugStrExecutablePointer - (uintptr_t)headerExecutablePointer;
    footer->sections.debug_str.size = jit->dwarfDebugInfoBuilder.str.size;

    footer->sections.debug_abbrev.offset = (uintptr_t)debugAbbrevExecutablePointer - (uintptr_t)headerExecutablePointer;
    footer->sections.debug_abbrev.size = jit->dwarfDebugInfoBuilder.abbrev.size;

    footer->sections.debug_info.offset = (uintptr_t)debugInfoExecutablePointer - (uintptr_t)headerExecutablePointer;
    footer->sections.debug_info.size = jit->dwarfDebugInfoBuilder.info.size;

    footer->sections.symtab.offset += contentOffset;
    footer->sections.str.offset += contentOffset;
    footer->sections.shstr.offset += contentOffset;
}


static void beacon_jit_emitDebugInfo(beacon_bytecodeJit_t *jit)
{
    bool hasLineInfo = beacon_jit_emitDebugLineInfo(jit);

    beacon_dwarf_debugInfo_beginDIE(&jit->dwarfDebugInfoBuilder, DW_TAG_compile_unit, true);
    beacon_dwarf_debugInfo_attribute_string(&jit->dwarfDebugInfoBuilder, DW_AT_producer, "Sysbvmi"); // Use the line info.
    if(hasLineInfo)
        beacon_dwarf_debugInfo_attribute_secOffset(&jit->dwarfDebugInfoBuilder, DW_AT_stmt_list, 0);
    beacon_dwarf_debugInfo_attribute_textAddress(&jit->dwarfDebugInfoBuilder, DW_AT_low_pc, 0);
    beacon_dwarf_debugInfo_attribute_textAddress(&jit->dwarfDebugInfoBuilder, DW_AT_high_pc, jit->instructions.size);
    beacon_dwarf_debugInfo_endDIE(&jit->dwarfDebugInfoBuilder);

    size_t oopTypeDie = beacon_dwarf_debugInfo_beginDIE(&jit->dwarfDebugInfoBuilder, DW_TAG_base_type, false);
    {
        beacon_dwarf_debugInfo_attribute_string(&jit->dwarfDebugInfoBuilder, DW_AT_name, "Oop");
        beacon_dwarf_debugInfo_attribute_uleb128(&jit->dwarfDebugInfoBuilder, DW_AT_encoding, DW_ATE_signed);
        beacon_dwarf_debugInfo_attribute_uleb128(&jit->dwarfDebugInfoBuilder, DW_AT_byte_size, sizeof(beacon_oop_t));
    }
    beacon_dwarf_debugInfo_endDIE(&jit->dwarfDebugInfoBuilder);

    {
        beacon_dwarf_debugInfo_beginDIE(&jit->dwarfDebugInfoBuilder, DW_TAG_subprogram, false);
        if(hasLineInfo && jit->sourcePosition)
        {
            beacon_SourcePosition_t *sourcePositionObject = (beacon_SourcePosition_t*)jit->sourcePosition;
            uint32_t line = beacon_decodeSmallInteger(sourcePositionObject->startLine);
            beacon_dwarf_debugInfo_attribute_uleb128(&jit->dwarfDebugInfoBuilder, DW_AT_decl_file, beacon_jit_dwarfLineInfoEmissionState_indexOfFile(&jit->dwarfLineEmissionState, sourcePositionObject->sourceCode));
            beacon_dwarf_debugInfo_attribute_uleb128(&jit->dwarfDebugInfoBuilder, DW_AT_decl_line, line);

        }
        beacon_dwarf_debugInfo_attribute_textAddress(&jit->dwarfDebugInfoBuilder, DW_AT_low_pc, 0);
        beacon_dwarf_debugInfo_attribute_textAddress(&jit->dwarfDebugInfoBuilder, DW_AT_high_pc, jit->instructions.size);
        beacon_dwarf_debugInfo_attribute_textAddress(&jit->dwarfDebugInfoBuilder, DW_AT_type, oopTypeDie);
        beacon_dwarf_debugInfo_attribute_beginLocationExpression(&jit->dwarfDebugInfoBuilder, DW_AT_frame_base);
        beacon_dwarf_debugInfo_location_register(&jit->dwarfDebugInfoBuilder, sizeof(uintptr_t) == 8 ? DW_X64_REG_RBP : DW_X86_REG_EBP);
        beacon_dwarf_debugInfo_attribute_endLocationExpression(&jit->dwarfDebugInfoBuilder);
        beacon_dwarf_debugInfo_endDIE(&jit->dwarfDebugInfoBuilder);
    }

    beacon_dwarf_debugInfo_endDIEChildren(&jit->dwarfDebugInfoBuilder);

    beacon_dwarf_debugInfo_finish(&jit->dwarfDebugInfoBuilder);
}

void beacon_jit_finish(beacon_bytecodeJit_t *jit)
{
    // Apply the PC target relative relocations.
    for(size_t i = 0; i < jit->pcRelocations.size; ++i)
    {
        beacon_bytecodeJitPCRelocation_t *relocation = beacon_DynArray_entryOfTypeAt(jit->pcRelocations, beacon_bytecodeJitPCRelocation_t, i);
        *((int32_t*)(jit->instructions.data + relocation->offset)) = (int32_t)(jit->pcDestinations[relocation->targetPC] - (intptr_t)relocation->offset + relocation->addend);
    }

    beacon_jit_emitUnwindInfo(jit);
    beacon_jit_emitDebugInfo(jit);
    beacon_jit_emitObjectFile(jit);
}

uint8_t *beacon_jit_installIn(beacon_bytecodeJit_t *jit, uint8_t *codeWriteablePointer, uint8_t *codeExecutablePointer)
{
    size_t objectFileHeaderOffset = 0;
    size_t codeOffset = beacon_sizeAlignedTo(jit->objectFileHeader.size, 16);
    size_t constantsOffset = codeOffset + beacon_sizeAlignedTo(jit->instructions.size, 16);

    size_t unwindInfoOffset = constantsOffset + beacon_sizeAlignedTo(jit->constants.size, 16);
    size_t ehFrameOffset = unwindInfoOffset + beacon_sizeAlignedTo(jit->unwindInfo.size, 16);
    size_t debugLineOffset = ehFrameOffset + beacon_sizeAlignedTo(jit->dwarfEhBuilder.buffer.size, 16);
    size_t debugStrOffset = debugLineOffset + beacon_sizeAlignedTo(jit->dwarfDebugInfoBuilder.line.size, 16);
    size_t debugAbbrevOffset = debugStrOffset + beacon_sizeAlignedTo(jit->dwarfDebugInfoBuilder.str.size, 16);
    size_t debugInfoOffset = debugAbbrevOffset + beacon_sizeAlignedTo(jit->dwarfDebugInfoBuilder.abbrev.size, 16);

    size_t objectFileContentOffset = debugInfoOffset + beacon_sizeAlignedTo(jit->dwarfDebugInfoBuilder.info.size, 16);

    uint8_t *objectFileHeaderPointer = codeWriteablePointer + objectFileHeaderOffset;
    uint8_t *objectFileHeaderExecutablePointer = codeExecutablePointer + objectFileHeaderOffset;
    memcpy(objectFileHeaderPointer, jit->objectFileHeader.data, jit->objectFileHeader.size);

    uint8_t *instructionsPointers = codeWriteablePointer + codeOffset;
    uint8_t *instructionsExecutablePointers = codeExecutablePointer + codeOffset;
    memcpy(instructionsPointers, jit->instructions.data, jit->instructions.size);

    uint8_t *constantZonePointer = codeWriteablePointer + constantsOffset;
    uint8_t *constantZoneExecutablePointer = codeExecutablePointer + constantsOffset;
    memcpy(constantZonePointer, jit->constants.data, jit->constants.size);

    for(size_t i = 0; i < jit->relocations.size; ++i)
    {
        beacon_bytecodeJitRelocation_t *relocation = beacon_DynArray_entryOfTypeAt(jit->relocations, beacon_bytecodeJitRelocation_t, i);
        uint8_t *relocationTarget = instructionsPointers + relocation->offset;
        uint8_t *relocationExecutableTarget = instructionsExecutablePointers + relocation->offset;
        intptr_t relocationTargetAddress = (intptr_t)relocationExecutableTarget;

        intptr_t relativeValue = (intptr_t)constantZoneExecutablePointer + relocation->value - relocationTargetAddress + relocation->addend;
        switch(relocation->type)
        {
        case BEACON_BYTECODE_JIT_RELOCATION_RELATIVE32:
            *((int32_t*)relocationTarget) = (int32_t)relativeValue;
            break;
        case BEACON_BYTECODE_JIT_RELOCATION_RELATIVE64:
            *((int64_t*)relocationTarget) = (int64_t)relativeValue;
            break;
        }
    }

    uint8_t *unwindInfoZonePointer = codeWriteablePointer + unwindInfoOffset;
    uint8_t *unwindInfoZoneExecutablePointer = codeExecutablePointer + unwindInfoOffset;
    memcpy(unwindInfoZonePointer, jit->unwindInfo.data, jit->unwindInfo.size);

    uint8_t *ehFrameZonePointer = codeWriteablePointer + ehFrameOffset;
    uint8_t *ehFrameZoneExecutablePointer = codeExecutablePointer + ehFrameOffset;
    memcpy(ehFrameZonePointer, jit->dwarfEhBuilder.buffer.data, jit->dwarfEhBuilder.buffer.size);

    beacon_dwarf_debugInfo_patchTextAddressesRelativeTo(&jit->dwarfDebugInfoBuilder, (uintptr_t)instructionsExecutablePointers);

    uint8_t *debugLineZonePointer = codeWriteablePointer + debugLineOffset;
    uint8_t *debugLineZoneExecutablePointer = codeExecutablePointer + debugLineOffset;
    memcpy(debugLineZonePointer, jit->dwarfDebugInfoBuilder.line.data, jit->dwarfDebugInfoBuilder.line.size);

    uint8_t *debugStrZonePointer = codeWriteablePointer + debugStrOffset;
    uint8_t *debugStrZoneExecutablePointer = codeExecutablePointer + debugStrOffset;
    memcpy(debugStrZonePointer, jit->dwarfDebugInfoBuilder.str.data, jit->dwarfDebugInfoBuilder.str.size);

    uint8_t *debugAbbrevZonePointer = codeWriteablePointer + debugAbbrevOffset;
    uint8_t *debugAbbrevZoneExecutablePointer = codeExecutablePointer + debugAbbrevOffset;
    memcpy(debugAbbrevZonePointer, jit->dwarfDebugInfoBuilder.abbrev.data, jit->dwarfDebugInfoBuilder.abbrev.size);

    uint8_t *debugInfoZonePointer = codeWriteablePointer + debugInfoOffset;
    uint8_t *debugInfoZoneExecutablePointer = codeExecutablePointer + debugInfoOffset;
    memcpy(debugInfoZonePointer, jit->dwarfDebugInfoBuilder.info.data, jit->dwarfDebugInfoBuilder.info.size);

    uint8_t *objectFileContentPointer = codeWriteablePointer + objectFileContentOffset;
    uint8_t *objectFileContentExecutablePointer = codeExecutablePointer + objectFileContentOffset;
    memcpy(objectFileContentPointer, jit->objectFileContent.data, jit->objectFileContent.size);

    beacon_jit_fixupObjectFile(jit,
        (beacon_elf64_header_t*)objectFileHeaderPointer,
        (beacon_elf64_header_t*)objectFileHeaderExecutablePointer,
        instructionsExecutablePointers,
        ehFrameZonePointer, ehFrameZoneExecutablePointer,
        debugLineZoneExecutablePointer,
        debugStrZoneExecutablePointer,
        debugAbbrevZoneExecutablePointer,
        debugInfoZoneExecutablePointer,
        objectFileContentExecutablePointer,
        (beacon_jit_x64_elfContentFooter_t*) (objectFileContentPointer + jit->objectFileContent.size - sizeof(beacon_jit_x64_elfContentFooter_t))
    );

#ifdef _WIN32
    RUNTIME_FUNCTION *runtimeFunction = (RUNTIME_FUNCTION*)unwindInfoZoneExecutablePointer;
    runtimeFunction->UnwindInfoAddress = (DWORD)(sizeof(RUNTIME_FUNCTION) + unwindInfoZoneExecutablePointer - instructionsExecutablePointers);
    if(RtlAddFunctionTable(runtimeFunction, 1, (DWORD64)(uintptr_t)instructionsExecutablePointers))
    {
        // Store the handle in the context for cleanup.
    }
#else
    (void)unwindInfoZoneExecutablePointer;
    if(jit->dwarfEhBuilder.buffer.size > 0)
    {
#   ifdef __APPLE__
        // It takes the FDE parameter
        if(jit->dwarfEhBuilder.fdeOffset > 0)
        {
            void *fdePointer = ehFrameZoneExecutablePointer + jit->dwarfEhBuilder.fdeOffset;
            beacon_DynArray_add(&jit->context->jittedRegisteredFrames, &fdePointer);
            __register_frame(fdePointer);
        }
#   else
        // Send the eh_frame section.
        beacon_DynArray_add(&jit->context->jittedRegisteredFrames, &ehFrameZoneExecutablePointer);
        __register_frame(ehFrameZoneExecutablePointer);
#   endif
    }
#endif

    return instructionsExecutablePointers;
}

#endif 