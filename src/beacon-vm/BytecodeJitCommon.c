#include "beacon-lang/BytecodeJit.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Exceptions.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef BEACON_JIT_SUPPORTED
void
beacon_bytecodeJit_initialize(beacon_bytecodeJit_t *jit, beacon_context_t *context)
{
    memset(jit, 0, sizeof(beacon_bytecodeJit_t));
    jit->context = context;
    beacon_DynArray_initialize(&jit->instructions, 1, 1024);
    beacon_DynArray_initialize(&jit->constants, 1, 1024);
    beacon_DynArray_initialize(&jit->relocations, sizeof(beacon_bytecodeJitRelocation_t), 0);
    beacon_DynArray_initialize(&jit->pcRelocations, sizeof(beacon_bytecodeJitPCRelocation_t), 0);
    beacon_DynArray_initialize(&jit->sourcePositions, sizeof(beacon_bytecodeJitSourcePositionRecord_t), 256);

    beacon_DynArray_initialize(&jit->unwindInfo, 1, 64);
    beacon_DynArray_initialize(&jit->unwindInfoBytecode, 1, 64);

    beacon_dwarf_cfi_create(&jit->dwarfEhBuilder);
    beacon_dwarf_debugInfo_create(&jit->dwarfDebugInfoBuilder);
    beacon_DynArray_initialize(&jit->objectFileHeader, 1, sizeof(beacon_elf64_header_t));
    beacon_DynArray_initialize(&jit->objectFileContent, 1, 1024);
}

size_t beacon_bytecodeJit_addBytes(beacon_bytecodeJit_t *jit, size_t byteCount, uint8_t *bytes)
{
    return beacon_DynArray_addAll(&jit->instructions, byteCount, bytes);
}

size_t beacon_bytecodeJit_addByte(beacon_bytecodeJit_t *jit, uint8_t byte)
{
    return beacon_bytecodeJit_addBytes(jit, 1, &byte);
}

size_t beacon_bytecodeJit_addConstantsBytes(beacon_bytecodeJit_t *jit, size_t byteCount, uint8_t *bytes)
{
    size_t offset = jit->constants.size;
    beacon_DynArray_addAll(&jit->constants, byteCount, bytes);
    return offset;
}

size_t beacon_bytecodeJit_addUnwindInfoBytes(beacon_bytecodeJit_t *jit, size_t byteCount, uint8_t *bytes)
{
    size_t offset = jit->unwindInfo.size;
    beacon_DynArray_addAll(&jit->unwindInfo, byteCount, bytes);
    return offset;
}

size_t beacon_bytecodeJit_addUnwindInfoByte(beacon_bytecodeJit_t *jit, uint8_t byte)
{
    return beacon_bytecodeJit_addUnwindInfoBytes(jit, 1, &byte);
}

#ifdef _WIN32
void beacon_bytecodeJit_uwop(beacon_bytecodeJit_t *jit, uint8_t opcode, uint8_t operationInfo)
{
    uint8_t prologueOffset = (uint8_t)jit->instructions.size;
    uint8_t operation = (operationInfo << 4) | opcode;
    uint16_t code = prologueOffset | (operation << 8);
    beacon_DynArray_addAll(&jit->unwindInfoBytecode, 2, &code);
}

void beacon_bytecodeJit_uwop_pushNonVol(beacon_bytecodeJit_t *jit, uint8_t reg)
{
    beacon_bytecodeJit_uwop(jit, /*UWOP_PUSH_NONVOL */0 , reg);
}

void beacon_bytecodeJit_uwop_setFPReg(beacon_bytecodeJit_t *jit)
{
    beacon_bytecodeJit_uwop(jit, /* UWOP_SET_FPREG */3, 0);
}

void beacon_bytecodeJit_uwop_alloc(beacon_bytecodeJit_t *jit, size_t amount)
{
    if(amount == 0) return;

    assert((amount % 8) == 0);
    if(amount <= 128)
    {
        beacon_bytecodeJit_uwop(jit, /* UWOP_ALLOC_SMALL */2, (uint8_t)(amount/8 - 1));
    }
    else if(amount <= 512*1024 - 8)
    {
        size_t encodedAmount = amount / 8;
        assert(encodedAmount <= 0xFFFF);
        uint16_t encodedAmountU16 = (uint16_t)encodedAmount;
        beacon_DynArray_addAll(&jit->unwindInfoBytecode, 2, &encodedAmountU16);
        beacon_bytecodeJit_uwop(jit, /* UWOP_ALLOC_LARGE */1, 0);
    }
    else
    {
        abort();
    }
}

#endif

void
beacon_bytecodeJit_addSourcePositionRecordWith(beacon_bytecodeJit_t *jit, size_t nativePC, beacon_SourcePosition_t *sourcePosition)
{
    if(jit->sourcePositions.size)
    {
        beacon_bytecodeJitSourcePositionRecord_t *existingRecords = (beacon_bytecodeJitSourcePositionRecord_t*)jit->sourcePositions.data;
        if(existingRecords[jit->sourcePositions.size - 1].sourcePosition == sourcePosition)
            return;
    }

    beacon_bytecodeJitSourcePositionRecord_t newRecord = {0};
    newRecord.sourcePosition = sourcePosition;
    newRecord.pc = nativePC;
    newRecord.line = beacon_decodeSmallInteger(sourcePosition->startLine);
    newRecord.column = beacon_decodeSmallInteger(sourcePosition->startColumn);
    beacon_DynArray_add(&jit->sourcePositions, &newRecord);
}

void
beacon_bytecodeJit_addSourcePositionRecord(beacon_bytecodeJit_t *jit, beacon_BytecodeCode_t *functionBytecode, uint16_t bytecodePC, size_t nativePC)
{
    /*if(functionBytecode->sourcePositions)
    {
        beacon_tuple_t foundDebugPosition = beacon_orderedOffsetTable_findValueWithOffset(jit->context, functionBytecode->debugSourcePositions, bytecodePC);
        if(foundDebugPosition)
        {
            beacon_bytecodeJit_addSourcePositionRecordWith(jit, nativePC, foundDebugPosition);
            return;
        }
    }

    if(functionBytecode->sourcePosition)
        beacon_bytecodeJit_addSourcePositionRecordWith(jit, nativePC, functionBytecode->sourcePosition);
    */
}

int
beacon_jit_dwarfLineInfoEmissionState_indexOfDirectory(beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_String_t *directory)
{
    for(int i = 0; i < state->directoryCount; ++i)
    {
        if(state->directories[i] == directory)
            return i + 1;
    }

    return 0;
}

void beacon_jit_dwarfLineInfoEmissionState_addDirectory(beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_String_t *directory)
{
    if(!beacon_jit_dwarfLineInfoEmissionState_indexOfDirectory(state, directory))
    {
        assert(state->directoryCount < BEACON_JIT_DWARF_LINE_INFO_EMISSION_STATE_MAX_DIRECTORIES);
        state->directories[state->directoryCount++] = directory;
    }
}

int beacon_jit_dwarfLineInfoEmissionState_indexOfFile(beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_SourceCode_t *file)
{
    for(int i = 0; i < state->fileCount; ++i)
    {
        if(state->files[i] == file)
            return i + 1;
    }

    return 0;
}

void beacon_jit_dwarfLineInfoEmissionState_addSourceCode(beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_SourceCode_t *sourceCode)
{
    if(!sourceCode)
        return;

    if(!beacon_jit_dwarfLineInfoEmissionState_indexOfFile(state, sourceCode))
    {
        beacon_jit_dwarfLineInfoEmissionState_addDirectory(state, sourceCode->directory);

        state->files[state->fileCount++] = sourceCode;
    }
}

void beacon_jit_dwarfLineInfoEmissionState_addSourcePositionSourceCode(beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_SourcePosition_t *sourcePosition)
{
    if(!sourcePosition)
        return;

    beacon_jit_dwarfLineInfoEmissionState_addSourceCode(state, sourcePosition->sourceCode);
}

void beacon_jit_dwarfLineInfoEmissionState_addSourcePosition(beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_SourcePosition_t *sourcePosition, uint32_t line)
{
    if(!sourcePosition)
        return;

    beacon_jit_dwarfLineInfoEmissionState_addSourceCode(state, sourcePosition->sourceCode);

    int lineAdvance = state->previousLine - line;
    if(abs(lineAdvance) < 8)
    {
        if(lineAdvance < state->minLineAdvance)
            state->minLineAdvance = lineAdvance;
        if(lineAdvance > state->maxLineAdvance)
            state->maxLineAdvance = lineAdvance;
    }

    state->previousLine = line;
}

void beacon_jit_dwarfLineInfoEmissionState_emitSourcePosition(beacon_bytecodeJit_t *jit, beacon_jit_dwarfLineInfoEmissionState_t *state, beacon_SourcePosition_t *sourcePosition, size_t pc, uint32_t line, uint32_t column)
{
    if(!sourcePosition)
        return;

    // Set the source code.
    if(sourcePosition->sourceCode != state->currentFile)
    {
        beacon_dwarf_debugInfo_line_setFile(&jit->dwarfDebugInfoBuilder, beacon_jit_dwarfLineInfoEmissionState_indexOfFile(state, sourcePosition->sourceCode));
        state->currentFile = sourcePosition->sourceCode;
    }

    // Set the column.
    beacon_dwarf_debugInfo_line_setColumn(&jit->dwarfDebugInfoBuilder, column);

    int pcAdvance = (int)(pc - state->pc);
    int lineAdvance = line - state->previousLine;
    beacon_dwarf_debugInfo_line_advanceLineAndPC(&jit->dwarfDebugInfoBuilder, lineAdvance, pcAdvance);
    state->pc = pc;
    state->previousLine = line;
}

bool beacon_jit_emitDebugLineInfo(beacon_bytecodeJit_t *jit)
{
    if(!jit->sourcePositions.size)
        return false;

    beacon_jit_dwarfLineInfoEmissionState_t *state = &jit->dwarfLineEmissionState;
    memset(state, 0, sizeof(*state));
    beacon_jit_dwarfLineInfoEmissionState_addSourcePositionSourceCode(state, jit->sourcePosition);

    // Preprocessing step.
    beacon_bytecodeJitSourcePositionRecord_t *sourcePositionRecords = (beacon_bytecodeJitSourcePositionRecord_t*)jit->sourcePositions.data;
    state->previousLine = 1;
    state->minLineAdvance = INT32_MAX;
    state->maxLineAdvance = INT32_MIN;
    for(size_t i = 0; i < jit->sourcePositions.size; ++i)
    {
        beacon_bytecodeJitSourcePositionRecord_t *record = sourcePositionRecords + i;
        beacon_jit_dwarfLineInfoEmissionState_addSourcePosition(state, record->sourcePosition, record->line);
    }

    if(state->minLineAdvance > state->maxLineAdvance)
        state->minLineAdvance = state->maxLineAdvance = 1;

    state->lineBase = state->minLineAdvance;
    state->lineRange = state->maxLineAdvance - state->minLineAdvance;
    if(state->lineRange < 1)
        state->lineRange = 1;

    jit->dwarfDebugInfoBuilder.lineProgramHeader.lineBase = state->lineBase;
    jit->dwarfDebugInfoBuilder.lineProgramHeader.lineRange = state->lineRange;

    beacon_dwarf_debugInfo_beginLineInformation(&jit->dwarfDebugInfoBuilder);

    // Directories
    for(int i = 0; i < state->directoryCount; ++i)
        beacon_dwarf_debugInfo_addDirectory(&jit->dwarfDebugInfoBuilder, state->directories[i]);
    beacon_dwarf_debugInfo_endDirectoryList(&jit->dwarfDebugInfoBuilder);

    // Files    
    for(int i = 0; i < state->fileCount; ++i)
    {
        beacon_SourceCode_t *sourceCode = state->files[i];
        beacon_dwarf_debugInfo_addFile(&jit->dwarfDebugInfoBuilder, beacon_jit_dwarfLineInfoEmissionState_indexOfDirectory(state, sourceCode->directory), sourceCode->name);
    }
    beacon_dwarf_debugInfo_endFileList(&jit->dwarfDebugInfoBuilder);

    beacon_dwarf_debugInfo_endLineInformationHeader(&jit->dwarfDebugInfoBuilder);

    // Emit the source positions.
    state->previousLine = 1;
    state->pc = 0;
    state->currentFile = NULL;
    if(state->fileCount > 0)
        state->currentFile = state->files[0];
    beacon_dwarf_debugInfo_line_setAddress(&jit->dwarfDebugInfoBuilder, state->pc);
    for(size_t i = 0; i < jit->sourcePositions.size; ++i)
    {
        beacon_bytecodeJitSourcePositionRecord_t *record = sourcePositionRecords + i;
        beacon_jit_dwarfLineInfoEmissionState_emitSourcePosition(jit, state, record->sourcePosition, record->pc, record->line, record->column);
    }

    beacon_dwarf_debugInfo_line_endSequence(&jit->dwarfDebugInfoBuilder);
    beacon_dwarf_debugInfo_endLineInformation(&jit->dwarfDebugInfoBuilder);

    return true;
}

void beacon_bytecodeJit_addPCRelocation(beacon_bytecodeJit_t *jit, beacon_bytecodeJitPCRelocation_t relocation)
{
    beacon_DynArray_add(&jit->pcRelocations, &relocation);
}

void beacon_bytecodeJit_addRelocation(beacon_bytecodeJit_t *jit, beacon_bytecodeJitRelocation_t relocation)
{
    beacon_DynArray_add(&jit->relocations, &relocation);
}

void beacon_bytecodeJit_jitFree(beacon_bytecodeJit_t *jit)
{
    beacon_DynArray_destroy(&jit->instructions);
    beacon_DynArray_destroy(&jit->constants);
    beacon_DynArray_destroy(&jit->relocations);
    beacon_DynArray_destroy(&jit->pcRelocations);
    beacon_DynArray_destroy(&jit->sourcePositions);
    free(jit->pcDestinations);

    beacon_DynArray_destroy(&jit->unwindInfo);
    beacon_DynArray_destroy(&jit->unwindInfoBytecode);
    beacon_dwarf_cfi_destroy(&jit->dwarfEhBuilder);
    beacon_dwarf_debugInfo_destroy(&jit->dwarfDebugInfoBuilder);
    
    beacon_DynArray_destroy(&jit->objectFileHeader);
    beacon_DynArray_destroy(&jit->objectFileContent);
}

bool beacon_bytecodeJit_jit(beacon_context_t *context, beacon_CompiledCode_t *function)
{
    return false;
    beacon_BytecodeCode_t *functionBytecode = function->bytecodeImplementation;
    if(!functionBytecode)
        return false;

    (void)context;
    beacon_bytecodeJit_t jit;
    beacon_bytecodeJit_initialize(&jit, context);

    uint8_t *bytecodes = functionBytecode->bytecodes->elements;
    size_t bytecodesSize = functionBytecode->bytecodes->super.super.super.super.super.header.slotCount;
    uint8_t extendedArgumentCount = 0;
    uint32_t pc = 0;
    
    beacon_oop_t bytecodeDecodedArguments[BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS];
    memset(bytecodeDecodedArguments, 0, sizeof(bytecodeDecodedArguments));

    while(pc < bytecodesSize)
    {
        uint32_t instructionPC = pc;
        int16_t branchDestinationDelta = 0;
        uint32_t branchDestinationPC = instructionPC;
        uint8_t instruction = bytecodes[pc++];

        uint8_t instructionArgumentCount = (extendedArgumentCount << 4) | beacon_getBytecodeArgumentCount(instruction);
        BeaconAssert(context, instructionArgumentCount <= BEACON_MAX_SUPPORTED_BYTECODE_ARGUMENTS);
        beacon_BytecodeOpcode_t opcode = beacon_getBytecodeOpcode(instruction);

        printf("%03d: %d\n", instructionPC, opcode);

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

            /*switch(beacon_BytecodeValue_getType(bytecodeArgument))
            {
            case BytecodeArgumentTypeArgument:
                BeaconAssert(context, bytecodeArgumentIndex <= argumentCount);
                if(bytecodeArgumentIndex == 0)
                    *currentDecodedArgument = receiver;
                else
                    *currentDecodedArgument = arguments[bytecodeArgumentIndex - 1];
                break;
            case BytecodeArgumentTypeLiteral:
            case BytecodeArgumentTypeSuperReceiver:
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
            */
        }

    }

    beacon_bytecodeJit_jitFree(&jit);
    return false;
#if 0

    jit.sourcePosition = function->sourcePosition;
    jit.compiledProgramEntity = function;

    jit.literalVectorGCRoot = beacon_heap_allocateGCRootTableEntry(&context->heap);
    *jit.literalVectorGCRoot = functionBytecode->literals;

    size_t instructionsSize = beacon_tuple_getSizeInBytes(functionBytecode->instructions);
    uint8_t *instructions = BEACON_CAST_OOP_TO_OBJECT_TUPLE(functionBytecode->instructions)->bytes;

    int16_t decodedOperands[BEACON_BYTECODE_FUNCTION_OPERAND_REGISTER_FILE_SIZE] = {0};
    jit.argumentCount = beacon_tuple_size_decode(functionBytecode->argumentCount);
    jit.captureVectorSize = beacon_tuple_size_decode(functionBytecode->captureVectorSize);
    jit.literalCount = beacon_tuple_getSizeInSlots(functionBytecode->literalVector);
    jit.localVectorSize = beacon_tuple_size_decode(functionBytecode->localVectorSize);

    jit.pcDestinations = (intptr_t*)malloc(sizeof(intptr_t)*instructionsSize);
    memset(jit.pcDestinations, -1, sizeof(intptr_t)*instructionsSize);

    beacon_jit_prologue(&jit);
    beacon_jit_finish(&jit);

    size_t objectFileHeaderSize = beacon_sizeAlignedTo(jit.objectFileHeader.size, 16);
    size_t textSectionSize = beacon_sizeAlignedTo(jit.instructions.size, 16);
    size_t rodataSectionSize = beacon_sizeAlignedTo(jit.constants.size, 16);
    size_t unwindInfoSize = beacon_sizeAlignedTo(jit.unwindInfo.size, 16)  + beacon_sizeAlignedTo(jit.dwarfEhBuilder.buffer.size, 16);
    size_t debugInfoSize = beacon_sizeAlignedTo(jit.dwarfDebugInfoBuilder.line.size, 16)
        + beacon_sizeAlignedTo(jit.dwarfDebugInfoBuilder.str.size, 16)
        + beacon_sizeAlignedTo(jit.dwarfDebugInfoBuilder.abbrev.size, 16)
        + beacon_sizeAlignedTo(jit.dwarfDebugInfoBuilder.info.size, 16);
    size_t objectFileContentSize = beacon_sizeAlignedTo(jit.objectFileContent.size, 16);

    size_t requiredCodeSize = objectFileHeaderSize + textSectionSize + rodataSectionSize + unwindInfoSize + debugInfoSize + objectFileContentSize;
    uint8_t *codeWriteablePointer = NULL;
    uint8_t *codeExecutablePointer = NULL;
    beacon_chunkedAllocator_allocateWithDualMapping(&context->heap.codeAllocator, requiredCodeSize, 16, (void**)&codeWriteablePointer, (void**)&codeExecutablePointer);
    if(!beacon_virtualMemory_lockCodePagesForWriting(codeWriteablePointer, codeExecutablePointer, requiredCodeSize))
        abort();

    memset(codeWriteablePointer + objectFileHeaderSize, 0xcc, textSectionSize); // int3;
    memset(codeWriteablePointer + objectFileHeaderSize + textSectionSize, 0, rodataSectionSize); // int3;
    uint8_t *entryPointPointer = beacon_jit_installIn(&jit, codeWriteablePointer, codeExecutablePointer);
    beacon_virtualMemory_unlockCodePagesForExecution(codeWriteablePointer, codeExecutablePointer, requiredCodeSize);

    // Register the object file with gdb.
    if(jit.objectFileHeader.size > 0 && jit.objectFileContent.size > 0)
    {
        beacon_gdb_jit_code_entry_t *entry = (beacon_gdb_jit_code_entry_t*)calloc(1, sizeof(beacon_gdb_jit_code_entry_t));
        beacon_DynArray_add(&context->jittedObjectFileEntries, &entry);
        beacon_gdb_registerObjectFile(entry, codeExecutablePointer, requiredCodeSize);
    }

    //functionBytecode->jittedCode = beacon_tuple_systemHandle_encode(context, (beacon_systemHandle_t)(uintptr_t)entryPointPointer);
    //functionBytecode->jittedCodeSessionToken = context->roots.sessionToken;

    // Patch the trampoline.
    beacon_jit_patchTrampolineWithRealEntryPoint(&jit, functionBytecode);

    beacon_bytecodeJit_jitFree(&jit);
#endif
}

#endif //BEACON_JIT_SUPPORTED
