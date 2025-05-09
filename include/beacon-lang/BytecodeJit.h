#ifndef BEACON_BYTECODE_JIT_H
#define BEACON_BYTECODE_JIT_H

#include "DynArray.h"
#include "Bytecode.h"
#include "Elf.h"
#include "Dwarf.h"

#if defined(__x86_64__) || defined(_M_X64)
#   define BEACON_ARCH_X86_64 1
#endif

#if defined(__aarch64__)
#   define BEACON_ARCH_AARCH64 1
#endif

#if defined(BEACON_ARCH_X86_64)
#   define BEACON_JIT_SUPPORTED 1
#endif

#ifdef BEACON_JIT_SUPPORTED

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <windows.h>
#endif

#define BEACON_JIT_DWARF_LINE_INFO_EMISSION_STATE_MAX_DIRECTORIES 8
#define BEACON_JIT_DWARF_LINE_INFO_EMISSION_STATE_MAX_FILES 8

typedef enum beacon_bytecodeJitRelocationType_e
{
    BEACON_BYTECODE_JIT_RELOCATION_RELATIVE32,
    BEACON_BYTECODE_JIT_RELOCATION_RELATIVE64,
} beacon_bytecodeJitRelocationType_t;

typedef struct beacon_bytecodeJitRelocation_s
{
    size_t offset;
    beacon_bytecodeJitRelocationType_t type;
    intptr_t value;
    intptr_t addend;
} beacon_bytecodeJitRelocation_t;

typedef struct beacon_bytecodeJitPCRelocation_s
{
    size_t offset;
    size_t targetPC;
    intptr_t addend;
} beacon_bytecodeJitPCRelocation_t;

typedef struct beacon_bytecodeJitSourcePositionRecord_s
{
    size_t pc;
    beacon_SourcePosition_t *sourcePosition;
    uint32_t line;
    uint32_t column;
} beacon_bytecodeJitSourcePositionRecord_t;

typedef struct beacon_jit_dwarfLineInfoEmissionState_s
{
    int directoryCount;
    beacon_String_t *directories[BEACON_JIT_DWARF_LINE_INFO_EMISSION_STATE_MAX_DIRECTORIES];

    int fileCount;
    beacon_SourceCode_t *files[BEACON_JIT_DWARF_LINE_INFO_EMISSION_STATE_MAX_FILES];

    int32_t previousLine;
    int32_t minLineAdvance;
    int32_t maxLineAdvance;
    int32_t lineBase;
    int32_t lineRange;

    beacon_SourceCode_t *currentFile;
    size_t pc;
} beacon_jit_dwarfLineInfoEmissionState_t;

typedef struct beacon_bytecodeJitDecodedArgument_s
{
    beacon_BytecodeValueType_t type;
    uint16_t index;
    int16_t signedIndex;
} beacon_bytecodeJitDecodedOperand_t;

typedef struct beacon_bytecodeJit_s
{
    beacon_context_t *context;
    beacon_SourcePosition_t *sourcePosition;
    beacon_CompiledCode_t *compiledProgramEntity;

    size_t argumentCount;
    size_t captureVectorSize;
    size_t literalCount;
    size_t localVectorSize;
    size_t maxCallArgumentCount;

    int32_t contextPointerOffset;
    int32_t localVectorOffset;
    int32_t argumentVectorOffset;
    int32_t literalVectorOffset;
    int32_t captureVectorOffset;
    int32_t pcOffset;
    int32_t stackFrameRecordOffset;
    int32_t callArgumentVectorSizeOffset;
    int32_t callArgumentVectorOffset;
    int32_t stackFrameSize;
    int32_t stackCallReservationSize;
    int32_t cfiFrameOffset;

    beacon_DynArray_t objectFileHeader;
    beacon_DynArray_t instructions;
    beacon_DynArray_t constants;
    beacon_DynArray_t relocations;
    beacon_DynArray_t pcRelocations;
    beacon_DynArray_t sourcePositions;
    beacon_DynArray_t unwindInfo;
    beacon_DynArray_t unwindInfoBytecode;
    beacon_dwarf_cfi_builder_t dwarfEhBuilder;
    beacon_dwarf_debugInfo_builder_t dwarfDebugInfoBuilder;
    beacon_DynArray_t objectFileContent;
    size_t objectFileContentJittedFunctionNameOffset;
    size_t prologueSize;

    beacon_jit_dwarfLineInfoEmissionState_t dwarfLineEmissionState;

    intptr_t *pcDestinations;

    beacon_Array_t *literalVector;
} beacon_bytecodeJit_t;

static inline size_t beacon_sizeAlignedTo(size_t pointer, size_t alignment)
{
    return (pointer + alignment - 1) & (~(alignment - 1));
}

int beacon_jit_dwarfLineInfoEmissionState_indexOfDirectory(beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_String_t *directory);
int beacon_jit_dwarfLineInfoEmissionState_indexOfFile(beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_SourceCode_t *file);

void beacon_bytecodeJit_initialize(beacon_bytecodeJit_t *jit, beacon_context_t *context);
size_t beacon_bytecodeJit_addBytes(beacon_bytecodeJit_t *jit, size_t byteCount, uint8_t *bytes);
size_t beacon_bytecodeJit_addByte(beacon_bytecodeJit_t *jit, uint8_t byte);
size_t beacon_bytecodeJit_addConstantsBytes(beacon_bytecodeJit_t *jit, size_t byteCount, uint8_t *bytes);
size_t beacon_bytecodeJit_addUnwindInfoBytes(beacon_bytecodeJit_t *jit, size_t byteCount, uint8_t *bytes);
size_t beacon_bytecodeJit_addUnwindInfoByte(beacon_bytecodeJit_t *jit, uint8_t byte);

#ifdef _WIN32
void beacon_bytecodeJit_uwop(beacon_bytecodeJit_t *jit, uint8_t opcode, uint8_t operationInfo);
void beacon_bytecodeJit_uwop_pushNonVol(beacon_bytecodeJit_t *jit, uint8_t reg);
void beacon_bytecodeJit_uwop_setFPReg(beacon_bytecodeJit_t *jit);
void beacon_bytecodeJit_uwop_alloc(beacon_bytecodeJit_t *jit, size_t amount);
#endif

void beacon_bytecodeJit_addPCRelocation(beacon_bytecodeJit_t *jit, beacon_bytecodeJitPCRelocation_t relocation);
void beacon_bytecodeJit_addRelocation(beacon_bytecodeJit_t *jit, beacon_bytecodeJitRelocation_t relocation);
void beacon_bytecodeJit_jitFree(beacon_bytecodeJit_t *jit);

bool beacon_bytecodeJit_jit(beacon_context_t *context, beacon_CompiledCode_t *functionBytecode);

// Backend specific methods.
void beacon_jit_prologue(beacon_bytecodeJit_t *jit);
bool beacon_jit_emitDebugLineInfo(beacon_bytecodeJit_t *jit);
void beacon_jit_finish(beacon_bytecodeJit_t *jit);
uint8_t *beacon_jit_installIn(beacon_bytecodeJit_t *jit, uint8_t *codeWriteablePointer, uint8_t *codeExecutablePointer);

void beacon_jit_breakpoint(beacon_bytecodeJit_t *jit);
void beacon_jit_unreachable(beacon_bytecodeJit_t *jit);

void beacon_jit_moveOperandToOperand(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t destinationOperand, beacon_bytecodeJitDecodedOperand_t sourceOperand);
void beacon_jit_return(beacon_bytecodeJit_t *jit, beacon_bytecodeJitDecodedOperand_t operand);
void beacon_jit_storePC(beacon_bytecodeJit_t *jit, uint16_t pc);


#endif //BEACON_JIT_SUPPORTED
#endif //BEACON_BYTECODE_JIT_H
