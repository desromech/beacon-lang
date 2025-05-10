#include "beacon-lang/Dwarf.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


size_t beacon_dwarf_encodeDwarfPointer(beacon_DynArray_t *buffer, uint32_t value)
{
    size_t offset = buffer->size;
    beacon_DynArray_addAll(buffer, sizeof(value), &value);
    return offset;
}

size_t beacon_dwarf_encodeDwarfPointerPCRelative(beacon_DynArray_t *buffer, uint32_t value)
{
    int32_t pcRelativeValue = (int32_t)(buffer->size - value);
    size_t offset = buffer->size;
    beacon_DynArray_addAll(buffer, sizeof(pcRelativeValue), &pcRelativeValue);
    return offset;
}

size_t beacon_dwarf_encodePointer(beacon_DynArray_t *buffer, uintptr_t value)
{
    size_t offset = buffer->size;
    beacon_DynArray_addAll(buffer, sizeof(value), &value);
    return offset;
}

size_t beacon_dwarf_encodeByte(beacon_DynArray_t *buffer, uint8_t value)
{
    size_t offset = buffer->size;
    beacon_DynArray_addAll(buffer, sizeof(value), &value);
    return offset;
}

size_t beacon_dwarf_encodeWord(beacon_DynArray_t *buffer, uint16_t value)
{
    size_t offset = buffer->size;
    beacon_DynArray_addAll(buffer, sizeof(value), &value);
    return offset;
}

size_t beacon_dwarf_encodeDWord(beacon_DynArray_t *buffer, uint32_t value)
{
    size_t offset = buffer->size;
    beacon_DynArray_addAll(buffer, sizeof(value), &value);
    return offset;
}

size_t beacon_dwarf_encodeQWord(beacon_DynArray_t *buffer, uint64_t value)
{
    size_t offset = buffer->size;
    beacon_DynArray_addAll(buffer, sizeof(value), &value);
    return offset;
}

size_t beacon_dwarf_encodeCString(beacon_DynArray_t *buffer, const char *cstring)
{
    size_t offset = buffer->size;
    beacon_DynArray_addAll(buffer, strlen(cstring) + 1, cstring);
    return offset;
}

size_t beacon_dwarf_encodeStringObjectWithDefaultString(beacon_DynArray_t *buffer, beacon_String_t *string, const char *defaultString)
{
    size_t offset = buffer->size;
    if(string->super.super.super.super.super.header.slotCount > 0)
    {
        beacon_DynArray_addAll(buffer, string->super.super.super.super.super.header.slotCount, string->data);
        beacon_dwarf_encodeByte(buffer, 0);
    }
    else
    {
        beacon_dwarf_encodeCString(buffer, defaultString);
    }

    return offset;
}

size_t beacon_dwarf_encodeULEB128(beacon_DynArray_t *buffer, uintptr_t value)
{
    size_t offset = buffer->size;
    uintptr_t currentValue = value;
    do
    {
        uint8_t byte = currentValue & 127;
        currentValue >>= 7;

        if(currentValue)
            byte |= 128;
        beacon_DynArray_add(buffer, &byte);
    } while (currentValue != 0);
    return offset;
}

size_t beacon_dwarf_encodeSLEB128(beacon_DynArray_t *buffer, intptr_t value)
{
    size_t offset = buffer->size;
    bool more = true;

    intptr_t currentValue = value;
    while(more)
    {
        uint8_t byte = currentValue & 127;
        currentValue >>= 7;
        
        bool byteHasSign = byte & 0x40;
        if ((currentValue == 0 && !byteHasSign) || (currentValue == -1 && byteHasSign))
            more = false;
        else
            byte = byte | 0x80;

        beacon_DynArray_add(buffer, &byte);
    }
    return offset;
}

size_t beacon_dwarf_encodeAlignment(beacon_DynArray_t *buffer, size_t alignment)
{
    size_t offset = buffer->size;
    size_t alignedSize = (buffer->size + alignment - 1) & (-alignment);
    size_t padding = alignedSize - buffer->size;
    for(size_t i = 0; i < padding; ++i)
        beacon_dwarf_encodeByte(buffer, 0);
    return offset;
}

void beacon_dwarf_cfi_create(beacon_dwarf_cfi_builder_t *cfi)
{
    memset(cfi, 0, sizeof(beacon_dwarf_cfi_builder_t));
    cfi->version = 1;
    cfi->isEhFrame = true;
    beacon_DynArray_initialize(&cfi->buffer, 1, 1024);
}

void beacon_dwarf_cfi_destroy(beacon_dwarf_cfi_builder_t *cfi)
{
    beacon_DynArray_destroy(&cfi->buffer);
}

void beacon_dwarf_cfi_beginCIE(beacon_dwarf_cfi_builder_t *cfi, beacon_dwarf_cie_t *cie)
{
    cfi->cieOffset = beacon_dwarf_encodeDWord(&cfi->buffer, 0);
    cfi->cieContentOffset = beacon_dwarf_encodeDwarfPointer(&cfi->buffer, cfi->isEhFrame ? 0 : -1 ); // CIE_id
    cfi->cie = *cie;
    beacon_dwarf_encodeByte(&cfi->buffer, cfi->version);
    beacon_dwarf_encodeCString(&cfi->buffer, cfi->isEhFrame ? "zR" : ""); // Argumentation
    if(!cfi->isEhFrame)
    {
        beacon_dwarf_encodeByte(&cfi->buffer, sizeof(uintptr_t)); // Address size
        beacon_dwarf_encodeByte(&cfi->buffer, 0); // Segment size
    }
    beacon_dwarf_encodeULEB128(&cfi->buffer, cie->codeAlignmentFactor);
    beacon_dwarf_encodeSLEB128(&cfi->buffer, cie->dataAlignmentFactor);
    if(cfi->version <= 2 && !cfi->isEhFrame)
        beacon_dwarf_encodeByte(&cfi->buffer, (uint8_t)cie->returnAddressRegister);
    else
        beacon_dwarf_encodeULEB128(&cfi->buffer, cie->returnAddressRegister);
    if(cfi->isEhFrame)
    {
        beacon_dwarf_encodeULEB128(&cfi->buffer, 1);
        beacon_dwarf_encodeByte(&cfi->buffer, DW_EH_PE_pcrel | DW_EH_PE_sdata4);
    }
}

void beacon_dwarf_cfi_endCIE(beacon_dwarf_cfi_builder_t *cfi)
{
    beacon_dwarf_encodeAlignment(&cfi->buffer, sizeof(uintptr_t));
    uint32_t cieSize = (uint32_t)(cfi->buffer.size - cfi->cieContentOffset);
    memcpy(cfi->buffer.data + cfi->cieOffset, &cieSize, 4);
}

void beacon_dwarf_cfi_beginFDE(beacon_dwarf_cfi_builder_t *cfi, size_t pc)
{
    cfi->fdeOffset = beacon_dwarf_encodeDWord(&cfi->buffer, 0);
    cfi->fdeContentOffset = beacon_dwarf_encodeDwarfPointerPCRelative(&cfi->buffer, (uint32_t)cfi->cieOffset);
    cfi->fdeInitialPC = pc;
    if(cfi->isEhFrame)
    {
        cfi->fdeInitialLocationOffset = beacon_dwarf_encodeDWord(&cfi->buffer, 0);
        cfi->fdeAddressingRangeOffset = beacon_dwarf_encodeDWord(&cfi->buffer, 0);
        beacon_dwarf_encodeULEB128(&cfi->buffer, 0);
    }
    else
    {
        cfi->fdeInitialLocationOffset = beacon_dwarf_encodePointer(&cfi->buffer, 0);
        cfi->fdeAddressingRangeOffset = beacon_dwarf_encodePointer(&cfi->buffer, 0);
    }
    cfi->currentPC = cfi->fdeInitialPC;
    cfi->stackFrameSize = cfi->initialStackFrameSize;
    cfi->framePointerRegister = 0;
    cfi->hasFramePointerRegister = false;
    cfi->isInPrologue = true;
}

void beacon_dwarf_cfi_endFDE(beacon_dwarf_cfi_builder_t *cfi, size_t pc)
{
    beacon_dwarf_encodeAlignment(&cfi->buffer, sizeof(uintptr_t));
    if(cfi->isEhFrame)
    {
        uint32_t pcRange = (uint32_t)(pc - cfi->fdeInitialPC);
        memcpy(cfi->buffer.data + cfi->fdeAddressingRangeOffset, &pcRange, sizeof(uint32_t));
    }
    else
    {
        uintptr_t pcRange = pc - cfi->fdeInitialPC;
        memcpy(cfi->buffer.data + cfi->fdeAddressingRangeOffset, &pcRange, sizeof(uintptr_t));
    }

    uint32_t fdeSize = (uint32_t)(cfi->buffer.size - cfi->fdeContentOffset);
    memcpy(cfi->buffer.data + cfi->fdeOffset, &fdeSize, 4);
}

void beacon_dwarf_cfi_finish(beacon_dwarf_cfi_builder_t *cfi)
{
    beacon_dwarf_encodeDWord(&cfi->buffer, 0);
}

void beacon_dwarf_cfi_setPC(beacon_dwarf_cfi_builder_t *cfi, size_t pc)
{
    size_t advance = pc - cfi->currentPC;
    if(advance)
    {
        size_t advanceFactor = advance / cfi->cie.codeAlignmentFactor;
        if(advanceFactor <= 63)
        {
            beacon_dwarf_encodeByte(&cfi->buffer, (DW_OP_CFA_advance_loc << 6) | (uint8_t)advanceFactor);
        }
        else
        {
            if(advanceFactor <= 0xFF)
            {
                beacon_dwarf_encodeByte(&cfi->buffer, DW_OP_CFA_advance_loc1);
                beacon_dwarf_encodeByte(&cfi->buffer, (uint8_t)advanceFactor);
            }
            else if(advanceFactor <= 0xFFFF)
            {
                beacon_dwarf_encodeByte(&cfi->buffer, DW_OP_CFA_advance_loc2);
                beacon_dwarf_encodeWord(&cfi->buffer, (uint16_t)advanceFactor);
            }
            else
            {
                assert(advanceFactor <= 0xFFFFFFFF);
                beacon_dwarf_encodeByte(&cfi->buffer, DW_OP_CFA_advance_loc4);
                beacon_dwarf_encodeDWord(&cfi->buffer, (uint32_t)advanceFactor);
            }
        }
    }

    cfi->currentPC = pc;
}

void beacon_dwarf_cfi_cfaInRegisterWithOffset(beacon_dwarf_cfi_builder_t *cfi, uintptr_t reg, intptr_t offset)
{
    beacon_dwarf_encodeByte(&cfi->buffer, DW_OP_CFA_def_cfa);
    beacon_dwarf_encodeULEB128(&cfi->buffer, reg);
    beacon_dwarf_encodeULEB128(&cfi->buffer, offset);
}

void beacon_dwarf_cfi_cfaInRegisterWithFactoredOffset(beacon_dwarf_cfi_builder_t *cfi, uintptr_t reg, size_t offset)
{
    beacon_dwarf_cfi_cfaInRegisterWithOffset(cfi, reg, sizeof(uintptr_t) * offset);
}

void beacon_dwarf_cfi_registerValueAtFactoredOffset(beacon_dwarf_cfi_builder_t *cfi, uintptr_t reg, size_t offset)
{
    if(reg <= 63) {
        beacon_dwarf_encodeByte(&cfi->buffer, (DW_OP_CFA_offset << 6) | (uint8_t)reg);
        beacon_dwarf_encodeULEB128(&cfi->buffer, offset);
    } else {
        beacon_dwarf_encodeByte(&cfi->buffer, DW_OP_CFA_offset_extended);
        beacon_dwarf_encodeULEB128(&cfi->buffer, reg);
        beacon_dwarf_encodeULEB128(&cfi->buffer, offset);
    }
}

void beacon_dwarf_cfi_pushRegister(beacon_dwarf_cfi_builder_t *cfi, uintptr_t reg)
{
    ++cfi->stackFrameSize;
    if(!cfi->hasFramePointerRegister)
        beacon_dwarf_cfi_cfaInRegisterWithFactoredOffset(cfi, cfi->stackPointerRegister, cfi->stackFrameSize);
    beacon_dwarf_cfi_registerValueAtFactoredOffset(cfi, reg, cfi->stackFrameSize);
}

void beacon_dwarf_cfi_saveFramePointerInRegister(beacon_dwarf_cfi_builder_t *cfi, uintptr_t reg, intptr_t offset)
{
    assert(!cfi->hasFramePointerRegister);
    assert((offset % sizeof(uintptr_t)) == 0);

    cfi->hasFramePointerRegister = true;
    cfi->framePointerRegister = reg;
    cfi->stackFrameSizeAtFramePointer = cfi->stackFrameSize - offset / sizeof(uintptr_t);
    beacon_dwarf_cfi_cfaInRegisterWithFactoredOffset(cfi, reg, cfi->stackFrameSizeAtFramePointer);
}

void beacon_dwarf_cfi_stackSizeAdvance(beacon_dwarf_cfi_builder_t *cfi, size_t pc, size_t increment)
{
    if(!cfi->isInPrologue) return;
    if(!increment) return;
    
    cfi->stackFrameSize += increment / sizeof(uintptr_t);
    if(!cfi->hasFramePointerRegister)
    {
        beacon_dwarf_cfi_setPC(cfi, pc);
        beacon_dwarf_cfi_cfaInRegisterWithFactoredOffset(cfi, cfi->stackPointerRegister, cfi->stackFrameSizeAtFramePointer);
    }
}

void beacon_dwarf_cfi_endPrologue(beacon_dwarf_cfi_builder_t *cfi)
{
    assert(cfi->isInPrologue);
    cfi->isInPrologue = false;
}

void beacon_dwarf_debugInfo_create(beacon_dwarf_debugInfo_builder_t *builder)
{
    memset(builder, 0, sizeof(beacon_dwarf_debugInfo_builder_t));
    builder->version = 4;
    builder->lineProgramHeader.minimumInstructionLength = 1;
    builder->lineProgramHeader.maximumOperationsPerInstruction = 1;
    builder->lineProgramHeader.opcodeBase = 13;
    builder->lineProgramHeader.defaultIsStatement = true;

    beacon_DynArray_initialize(&builder->locationExpression, 1, 256);

    beacon_DynArray_initialize(&builder->line, 1, 1024);
    beacon_DynArray_initialize(&builder->str, 1, 1024);
    beacon_DynArray_initialize(&builder->abbrev, 1, 1024);
    beacon_DynArray_initialize(&builder->info, 1, 1024);
    beacon_DynArray_initialize(&builder->lineTextAddresses, sizeof(uint32_t), 1024);
    beacon_DynArray_initialize(&builder->infoTextAddresses, sizeof(uint32_t), 32);

    // Null string.
    beacon_dwarf_encodeByte(&builder->str, 0);

    // Info header
    beacon_dwarf_encodeDwarfPointer(&builder->info, 0);
    beacon_dwarf_encodeWord(&builder->info, builder->version);
    beacon_dwarf_encodeDwarfPointer(&builder->info, 0); // Debug abbrev offset
    beacon_dwarf_encodeByte(&builder->info, sizeof(uintptr_t)); // Address size.
}

void beacon_dwarf_debugInfo_destroy(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_DynArray_destroy(&builder->locationExpression);
    beacon_DynArray_destroy(&builder->line);
    beacon_DynArray_destroy(&builder->str);
    beacon_DynArray_destroy(&builder->abbrev);
    beacon_DynArray_destroy(&builder->info);
    beacon_DynArray_destroy(&builder->lineTextAddresses);
    beacon_DynArray_destroy(&builder->infoTextAddresses);
}

void beacon_dwarf_debugInfo_finish(beacon_dwarf_debugInfo_builder_t *builder)
{
    // End the abbreviations.
    beacon_dwarf_encodeByte(&builder->abbrev, 0);

    // Info initial length.
    {
        uint32_t infoInitialLength = (uint32_t)(builder->info.size - 4);
        memcpy(builder->info.data, &infoInitialLength, 4);
    }
}

void beacon_dwarf_debugInfo_patchTextAddressesRelativeTo(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t baseAddress)
{
    uint32_t *lineOffsets = (uint32_t*)builder->lineTextAddresses.data;
    for(size_t i = 0; i < builder->lineTextAddresses.size; ++i)
    {
        uintptr_t *address = (uintptr_t *)(builder->line.data + lineOffsets[i]);
        *address += baseAddress;
    }

    uint32_t *infoOffsets = (uint32_t*)builder->infoTextAddresses.data;
    for(size_t i = 0; i < builder->infoTextAddresses.size; ++i)
    {
        uintptr_t *address = (uintptr_t *)(builder->info.data + infoOffsets[i]);
        *address += baseAddress;
    }
}

void beacon_dwarf_debugInfo_beginLineInformation(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeDWord(&builder->line, 0);
    beacon_dwarf_encodeWord(&builder->line, builder->version);
    builder->lineHeaderLengthOffset = (uint32_t)beacon_dwarf_encodeDWord(&builder->line, 0); // Header length
    beacon_dwarf_encodeByte(&builder->line, builder->lineProgramHeader.minimumInstructionLength);
    beacon_dwarf_encodeByte(&builder->line, builder->lineProgramHeader.maximumOperationsPerInstruction);
    beacon_dwarf_encodeByte(&builder->line, builder->lineProgramHeader.defaultIsStatement);
    beacon_dwarf_encodeByte(&builder->line, builder->lineProgramHeader.lineBase);
    beacon_dwarf_encodeByte(&builder->line, builder->lineProgramHeader.lineRange);
    beacon_dwarf_encodeByte(&builder->line, builder->lineProgramHeader.opcodeBase);

    beacon_dwarf_encodeByte(&builder->line, 0); // DW_LNS_copy
    beacon_dwarf_encodeByte(&builder->line, 1); // DW_LNS_advance_pc
    beacon_dwarf_encodeByte(&builder->line, 1); // DW_LNS_advance_line
    beacon_dwarf_encodeByte(&builder->line, 1); // DW_LNS_set_file
    beacon_dwarf_encodeByte(&builder->line, 1); // DW_LNS_set_column
    beacon_dwarf_encodeByte(&builder->line, 0); // DW_LNS_negate_stmt
    beacon_dwarf_encodeByte(&builder->line, 0); // DW_LNS_set_basic_block
    beacon_dwarf_encodeByte(&builder->line, 0); // DW_LNS_const_add_pc
    beacon_dwarf_encodeByte(&builder->line, 1); // DW_LNS_fixed_advance_pc
    beacon_dwarf_encodeByte(&builder->line, 0); // DW_LNS_set_prologue_end
    beacon_dwarf_encodeByte(&builder->line, 0); // DW_LNS_set_epilogue_begin
    beacon_dwarf_encodeByte(&builder->line, 1); // DW_LNS_set_isa

    builder->lineProgramState.regAddress = 0;
    builder->lineProgramState.regOpIndex = 0;
    builder->lineProgramState.regFile = 1;
    builder->lineProgramState.regLine = 1;
    builder->lineProgramState.regColumn = 0;
    builder->lineProgramState.regIsStatement = builder->lineProgramHeader.defaultIsStatement;
    builder->lineProgramState.regBasicBlock = false;
    builder->lineProgramState.regEndSequence = false;
    builder->lineProgramState.regPrologueEnd = false;
    builder->lineProgramState.regEpilogueBegin = false;
    builder->lineProgramState.regISA = 0;
    builder->lineProgramState.regDiscriminator = false;
}

void beacon_dwarf_debugInfo_addDirectory(beacon_dwarf_debugInfo_builder_t *builder, beacon_String_t *directoryName)
{
    beacon_dwarf_encodeStringObjectWithDefaultString(&builder->line, directoryName, ".");
}

void beacon_dwarf_debugInfo_endDirectoryList(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeByte(&builder->line, 0);
}

void beacon_dwarf_debugInfo_addFile(beacon_dwarf_debugInfo_builder_t *builder, int directoryIndex, beacon_String_t *name)
{
    beacon_dwarf_encodeStringObjectWithDefaultString(&builder->line, name, "<unknown>");
    beacon_dwarf_encodeULEB128(&builder->line, directoryIndex);
    beacon_dwarf_encodeULEB128(&builder->line, 0); // Last modification time.
    beacon_dwarf_encodeULEB128(&builder->line, 0); // Size in bytes.
}

void beacon_dwarf_debugInfo_endFileList(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeByte(&builder->line, 0);
}

void beacon_dwarf_debugInfo_endLineInformationHeader(beacon_dwarf_debugInfo_builder_t *builder)
{
    uint32_t headerSize = (uint32_t)(builder->line.size - builder->lineHeaderLengthOffset - 4);
    memcpy(builder->line.data + builder->lineHeaderLengthOffset, &headerSize, 4);
}

void beacon_dwarf_debugInfo_line_setAddress(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t value)
{
    beacon_dwarf_encodeByte(&builder->line, 0);
    beacon_dwarf_encodeULEB128(&builder->line, 1 + sizeof(value));
    beacon_dwarf_encodeByte(&builder->line, DW_LNE_set_address);
    uint32_t addressOffset = (uint32_t)beacon_dwarf_encodePointer(&builder->line, value);
    beacon_DynArray_add(&builder->lineTextAddresses, &addressOffset);
    builder->lineProgramState.regAddress = (uint32_t)value;
}

void beacon_dwarf_debugInfo_line_setFile(beacon_dwarf_debugInfo_builder_t *builder, uint32_t file)
{
    if(builder->lineProgramState.regFile == file)
        return;

    beacon_dwarf_encodeByte(&builder->line, DW_LNS_set_file);
    beacon_dwarf_encodeULEB128(&builder->line, file);
    builder->lineProgramState.regFile = file;
}

void beacon_dwarf_debugInfo_line_setColumn(beacon_dwarf_debugInfo_builder_t *builder, int column)
{
    if(builder->lineProgramState.regColumn == column)
        return;

    beacon_dwarf_encodeByte(&builder->line, DW_LNS_set_column);
    beacon_dwarf_encodeULEB128(&builder->line, column);
    builder->lineProgramState.regColumn = column;
}

void beacon_dwarf_debugInfo_line_advanceLine(beacon_dwarf_debugInfo_builder_t *builder, int deltaLine)
{
    if(deltaLine == 0)
        return;

    beacon_dwarf_encodeByte(&builder->line, DW_LNS_advance_line);
    beacon_dwarf_encodeSLEB128(&builder->line, deltaLine);
    builder->lineProgramState.regLine += deltaLine;
}

void beacon_dwarf_debugInfo_line_advancePC(beacon_dwarf_debugInfo_builder_t *builder, int deltaPC)
{
    if(deltaPC == 0)
        return;

    beacon_dwarf_encodeByte(&builder->line, DW_LNS_advance_pc);
    beacon_dwarf_encodeULEB128(&builder->line, deltaPC);
    builder->lineProgramState.regAddress += deltaPC;
}

void beacon_dwarf_debugInfo_line_copyRow(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeByte(&builder->line, DW_LNS_copy);
    builder->lineProgramState.regDiscriminator = false;
    builder->lineProgramState.regBasicBlock = false;
    builder->lineProgramState.regPrologueEnd = false;
    builder->lineProgramState.regEpilogueBegin = false;
}

void beacon_dwarf_debugInfo_line_advanceLineAndPC(beacon_dwarf_debugInfo_builder_t *builder, int deltaLine, int deltaPC)
{
    int operationAdvance = deltaPC / builder->lineProgramHeader.minimumInstructionLength;

    int opcode = (deltaLine - builder->lineProgramHeader.lineBase) + (builder->lineProgramHeader.lineRange * operationAdvance) + builder->lineProgramHeader.opcodeBase;
    if( (0 <= opcode) && (opcode <= 255) 
        && (deltaLine - builder->lineProgramHeader.lineBase < builder->lineProgramHeader.lineRange)
        && (deltaLine >= builder->lineProgramHeader.lineBase) )
    {
        beacon_dwarf_encodeByte(&builder->line, opcode);
        builder->lineProgramState.regLine += deltaLine;
        builder->lineProgramState.regAddress += deltaPC;
    }
    else
    {
        beacon_dwarf_debugInfo_line_advanceLine(builder, deltaLine);
        beacon_dwarf_debugInfo_line_advancePC(builder, deltaPC);
        beacon_dwarf_debugInfo_line_copyRow(builder);
    }
}

void beacon_dwarf_debugInfo_line_endSequence(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeByte(&builder->line, 0);
    beacon_dwarf_encodeULEB128(&builder->line, 1);
    beacon_dwarf_encodeByte(&builder->line, DW_LNE_end_sequence);
}

void beacon_dwarf_debugInfo_endLineInformation(beacon_dwarf_debugInfo_builder_t *builder)
{
    uint32_t lineInfoSize = (uint32_t)(builder->line.size - 4);
    memcpy(builder->line.data, &lineInfoSize, 4);
}

size_t beacon_dwarf_debugInfo_beginDIE(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t tag, bool hasChildren)
{
    int abbreviationCode = ++builder->abbreviationCount;
    beacon_dwarf_encodeULEB128(&builder->abbrev, abbreviationCode);
    beacon_dwarf_encodeULEB128(&builder->abbrev, tag);
    beacon_dwarf_encodeByte(&builder->abbrev, hasChildren ? DW_CHILDREN_yes : DW_CHILDREN_no);

    return beacon_dwarf_encodeULEB128(&builder->info, abbreviationCode);
}

void beacon_dwarf_debugInfo_endDIE(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeByte(&builder->abbrev, 0);
    beacon_dwarf_encodeByte(&builder->abbrev, 0);
}

void beacon_dwarf_debugInfo_endDIEChildren(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeULEB128(&builder->info, 0);
}

void beacon_dwarf_debugInfo_attribute_uleb128(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t attribute, uintptr_t value)
{
    beacon_dwarf_encodeULEB128(&builder->abbrev, attribute);
    beacon_dwarf_encodeULEB128(&builder->abbrev, DW_FORM_udata);

    beacon_dwarf_encodeULEB128(&builder->info, value);
}

void beacon_dwarf_debugInfo_attribute_secOffset(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t attribute, uintptr_t value)
{
    beacon_dwarf_encodeULEB128(&builder->abbrev, attribute);
    beacon_dwarf_encodeULEB128(&builder->abbrev, DW_FORM_sec_offset);

    beacon_dwarf_encodeDWord(&builder->info, (uint32_t)value);
}

void beacon_dwarf_debugInfo_attribute_string(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t attribute, const char *value)
{
    beacon_dwarf_encodeULEB128(&builder->abbrev, attribute);
    beacon_dwarf_encodeULEB128(&builder->abbrev, DW_FORM_strp);

    size_t stringOffset = beacon_dwarf_encodeCString(&builder->str, value);
    beacon_dwarf_encodeDWord(&builder->info, (uint32_t)stringOffset);
}

void beacon_dwarf_debugInfo_attribute_stringTupleWithDefaultString(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t attribute, beacon_String_t *value, const char *defaultString)
{
    beacon_dwarf_encodeULEB128(&builder->abbrev, attribute);
    beacon_dwarf_encodeULEB128(&builder->abbrev, DW_FORM_strp);

    size_t stringOffset = beacon_dwarf_encodeStringObjectWithDefaultString(&builder->str, value, defaultString);
    beacon_dwarf_encodeDWord(&builder->info, (uint32_t)stringOffset);
}

void beacon_dwarf_debugInfo_attribute_ref1(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t attribute, uint8_t value)
{
    beacon_dwarf_encodeULEB128(&builder->abbrev, attribute);
    beacon_dwarf_encodeULEB128(&builder->abbrev, DW_FORM_ref1);
    beacon_dwarf_encodeByte(&builder->info, value);
}

void beacon_dwarf_debugInfo_attribute_textAddress(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t attribute, uintptr_t value)
{
    beacon_dwarf_encodeULEB128(&builder->abbrev, attribute);
    beacon_dwarf_encodeULEB128(&builder->abbrev, DW_FORM_addr);

    uint32_t addressOffset = (uint32_t)beacon_dwarf_encodePointer(&builder->info, value);
    beacon_DynArray_add(&builder->infoTextAddresses, &addressOffset);
}

void beacon_dwarf_debugInfo_attribute_beginLocationExpression(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t attribute)
{
    beacon_dwarf_encodeULEB128(&builder->abbrev, attribute);
    beacon_dwarf_encodeULEB128(&builder->abbrev, DW_FORM_exprloc);
}

void beacon_dwarf_debugInfo_attribute_endLocationExpression(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeULEB128(&builder->info, builder->locationExpression.size);
    beacon_DynArray_addAll(&builder->info, builder->locationExpression.size, builder->locationExpression.data);
    builder->locationExpression.size = 0;
}

void beacon_dwarf_debugInfo_location_constUnsigned(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t constant)
{
    if(constant <= 31)
    {
        beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_lit0 + (uint8_t)constant);
        return;
    }

    if(constant <= 0xFF)
    {
        beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_const1u);
        beacon_dwarf_encodeByte(&builder->locationExpression, (uint8_t)constant);
        return;
    }

    if(constant <= 0xFFFF)
    {
        beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_const2u);
        beacon_dwarf_encodeWord(&builder->locationExpression, (uint16_t)constant);
        return;
    }

    if(constant <= 0xFFFFFFFF)
    {
        beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_const4u);
        beacon_dwarf_encodeDWord(&builder->locationExpression, (uint32_t)constant);
        return;
    }

    beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_const8u);
    beacon_dwarf_encodeQWord(&builder->locationExpression, (uint64_t)constant);
}

void beacon_dwarf_debugInfo_location_deref(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_deref);
}

void beacon_dwarf_debugInfo_location_frameBaseOffset(beacon_dwarf_debugInfo_builder_t *builder, intptr_t offset)
{
    beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_fbreg);
    beacon_dwarf_encodeSLEB128(&builder->locationExpression, offset);
}

void beacon_dwarf_debugInfo_location_plus(beacon_dwarf_debugInfo_builder_t *builder)
{
    beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_plus);
}

void beacon_dwarf_debugInfo_location_register(beacon_dwarf_debugInfo_builder_t *builder, uintptr_t reg)
{
    if(reg <= 31)
    {
        beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_reg0 + (uint8_t)reg);
    }
    else
    {
        beacon_dwarf_encodeByte(&builder->locationExpression, DW_OP_regx);
        beacon_dwarf_encodeULEB128(&builder->locationExpression, reg);
    }
}
