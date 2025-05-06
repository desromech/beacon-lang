#include "beacon-lang/BytecodeJit.h"

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
static void beacon_jit_moveRegisterToOperand(beacon_bytecodeJit_t *jit, int16_t operand, beacon_x86_register_t reg);
static void beacon_jit_moveOperandToRegister(beacon_bytecodeJit_t *jit, beacon_x86_register_t reg, int16_t operand);
static void beacon_jit_moveOperandToCallArgumentVector(beacon_bytecodeJit_t *jit, int16_t operand, int32_t callArgumentVectorIndex);

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


#endif 