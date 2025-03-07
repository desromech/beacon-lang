#include "beacon-lang/SourceCode.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Scanner.h"
#include "beacon-lang/SyntaxCompiler.h"
#include "beacon-lang/Parser.h"
#include "beacon-lang/ArrayList.h"
#include "beacon-lang/Exceptions.h"
#include <string.h>
#include <stdio.h>

void beacon_splitFileName(beacon_context_t *context, const char *inFileName, beacon_String_t **outDirectory, beacon_String_t **outBasename)
{
    int32_t separatorIndex = -1;
    int32_t i;
    for (i = 0; inFileName[i] != 0; ++i)
    {
        char c = inFileName[i];
        if(c == '/' || c == '\\')
            separatorIndex = i;
    }
    int32_t stringSize = i;
    if(separatorIndex < 0)
    {
        *outDirectory = beacon_importCString(context, ".");
        *outBasename = beacon_importCString(context, inFileName);
    }


    beacon_String_t *directory = beacon_allocateObjectWithBehavior(context->heap, context->classes.stringClass, sizeof(beacon_String_t) + separatorIndex + 1, BeaconObjectKindBytes);
    memcpy(directory->data, inFileName, separatorIndex + 1);

    beacon_String_t *basename = beacon_allocateObjectWithBehavior(context->heap, context->classes.stringClass, sizeof(beacon_String_t) + stringSize - separatorIndex + 1, BeaconObjectKindBytes);
    memcpy(basename->data, inFileName + separatorIndex + 1, stringSize - separatorIndex);
    
    *outDirectory = directory;
    *outBasename = basename;
}

beacon_SourceCode_t *beacon_makeSourceCodeFromFileNamed(beacon_context_t *context, const char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if(!file)
    {
        perror("Failed to open input file.");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    beacon_String_t *fileData = beacon_allocateObjectWithBehavior(context->heap, context->classes.stringClass, sizeof(beacon_String_t) + fileSize, BeaconObjectKindBytes);
    if(fileSize > 0 && fread(fileData->data, fileSize, 1, file) != 1)
    {
        perror("Failed to read input file data.");
        fclose(file);
    }
    fclose(file);

    beacon_SourceCode_t *sourceCode = beacon_allocateObjectWithBehavior(context->heap, context->classes.sourceCodeClass, sizeof(beacon_SourceCode_t), BeaconObjectKindPointers);
    beacon_splitFileName(context, fileName, &sourceCode->directory, &sourceCode->name);
    sourceCode->text = fileData;
    sourceCode->textSize = beacon_encodeSmallInteger(fileSize);
    return sourceCode;
}

beacon_SourceCode_t *beacon_makeSourceCodeFromString(beacon_context_t *context, const char *name, const char *string)
{
    beacon_String_t *importedName = beacon_importCString(context, name);
    beacon_String_t *importedString = beacon_importCString(context, string);
    size_t stringSize = strlen(string);
    beacon_SourceCode_t *sourceCode = beacon_allocateObjectWithBehavior(context->heap, context->classes.sourceCodeClass, sizeof(beacon_SourceCode_t), BeaconObjectKindPointers);
    sourceCode->name = importedName;
    sourceCode->text = importedString;
    sourceCode->textSize = beacon_encodeSmallInteger(stringSize);
    return sourceCode;
}

beacon_SourcePosition_t *beacon_sourcePosition_to(beacon_context_t *context, beacon_SourcePosition_t *start, beacon_SourcePosition_t *end)
{
    beacon_SourcePosition_t *merged = beacon_allocateObjectWithBehavior(context->heap, context->classes.sourcePositionClass, sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
    merged->sourceCode  = start->sourceCode;
    merged->startIndex  = start->startIndex;
    merged->startLine   = start->startLine;
    merged->startColumn = start->startColumn;
    merged->endIndex    = end->endIndex;
    merged->endLine     = end->endLine;
    merged->endColumn   = end->endColumn;
    return merged;
}

beacon_SourcePosition_t *beacon_sourcePosition_until(beacon_context_t *context, beacon_SourcePosition_t *start, beacon_SourcePosition_t *end)
{
    beacon_SourcePosition_t *merged = beacon_allocateObjectWithBehavior(context->heap, context->classes.sourcePositionClass, sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
    merged->sourceCode  = start->sourceCode;
    merged->startIndex  = start->startIndex;
    merged->startLine   = start->startLine;
    merged->startColumn = start->startColumn;
    merged->endIndex    = end->startIndex;
    merged->endLine     = end->startLine;
    merged->endColumn   = end->startColumn;
    return merged;
}

beacon_oop_t beacon_evaluateSourceCode(beacon_context_t *context, beacon_SourceCode_t *sourceCode)
{
    beacon_StackFrameRecord_t frameRecord = {
        .kind = StackFrameSourceCompilationRoots,
        .context = context,
        .sourceCompilationRoots.sourceCode = (beacon_oop_t)sourceCode
    };
    beacon_pushStackFrameRecord(&frameRecord);

    beacon_ArrayList_t *scannedSource = beacon_scanSourceCode(context, sourceCode);
    frameRecord.sourceCompilationRoots.tokenList = (beacon_oop_t)scannedSource;

    intptr_t tokenCount = beacon_ArrayList_size(scannedSource);
    for(intptr_t i = 1; i <= tokenCount; ++i)
    {
        beacon_ScannerToken_t *token = (beacon_ScannerToken_t *)beacon_ArrayList_at(context, scannedSource, i);
        beacon_TokenKind_t kind = beacon_decodeSmallInteger(token->kind);
        if(kind == BeaconTokenError)
            beacon_exception_scannerError(context, token);
           
        //printf("Token %d: %s\n", (int)i, beacon_TokenKind_toString());
    }

    beacon_ParseTreeNode_t *parseTree = beacon_parseWorkspaceTokenList(context, sourceCode, scannedSource);
    frameRecord.sourceCompilationRoots.parseTree = (beacon_oop_t)parseTree;

    // Disable GC during file evaluation.
    //beacon_memoryHeapDisableGC(context->heap);
    frameRecord.sourceCompilationRoots.evaluation = beacon_evaluateFileSyntax(context, parseTree, sourceCode);
    //beacon_memoryHeapEnableGC(context->heap);

    beacon_popStackFrameRecord(&frameRecord);
    return frameRecord.sourceCompilationRoots.evaluation;
}

static beacon_oop_t beacon_SourceCode_scan(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 0);
    beacon_SourceCode_t *sourceCode = (beacon_SourceCode_t*)receiver;
    beacon_ArrayList_t *scannedSource = beacon_scanSourceCode(context, sourceCode);

    return (beacon_oop_t)scannedSource;
}

static beacon_oop_t beacon_SourceCode_parseScannedSource(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 1);
    beacon_SourceCode_t *sourceCode = (beacon_SourceCode_t*)receiver;
    beacon_ArrayList_t *scannedSource = (beacon_ArrayList_t*)arguments[0];

    beacon_ParseTreeNode_t *parsedSource = beacon_parseWorkspaceTokenList(context, sourceCode, scannedSource);
    return (beacon_oop_t)parsedSource;
}

static beacon_oop_t beacon_SourceCode_evaluateFileSyntaxWithParsedCode(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 1);
    beacon_SourceCode_t *sourceCode = (beacon_SourceCode_t*)receiver;
    beacon_ParseTreeNode_t *parseTreeNode = (beacon_ParseTreeNode_t*)arguments[0];

    return beacon_evaluateFileSyntax(context, parseTreeNode, sourceCode);
}


void beacon_context_registerSourceCodePrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.sourceCodeClass, "scan", 0, beacon_SourceCode_scan);
    beacon_addPrimitiveToClass(context, context->classes.sourceCodeClass, "parseScannedSource:", 1, beacon_SourceCode_parseScannedSource);
    beacon_addPrimitiveToClass(context, context->classes.sourceCodeClass, "evaluateFileSyntaxWithParsedCode:", 1, beacon_SourceCode_evaluateFileSyntaxWithParsedCode);
    
}