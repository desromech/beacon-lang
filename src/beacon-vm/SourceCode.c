#include "beacon-lang/SourceCode.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/Memory.h"
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


    beacon_String_t *directory = beacon_allocateObjectWithBehavior(context->heap, context->roots.stringClass, sizeof(beacon_String_t) + separatorIndex + 2, BeaconObjectKindBytes);
    memcpy(directory->data, inFileName, separatorIndex + 1);

    beacon_String_t *basename = beacon_allocateObjectWithBehavior(context->heap, context->roots.stringClass, sizeof(beacon_String_t) + stringSize - separatorIndex + 1, BeaconObjectKindBytes);
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

    beacon_String_t *fileData = beacon_allocateObjectWithBehavior(context->heap, context->roots.stringClass, sizeof(beacon_String_t) + fileSize, BeaconObjectKindBytes);
    if(fread(fileData->data, fileSize, 1, file) != 1)
    {
        perror("Failed to read input file data.");
        fclose(file);
    }
    fclose(file);

    beacon_SourceCode_t *sourceCode = beacon_allocateObjectWithBehavior(context->heap, context->roots.stringClass, sizeof(beacon_SourceCode_t), BeaconObjectKindPointers);
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
    beacon_SourceCode_t *sourceCode = beacon_allocateObjectWithBehavior(context->heap, context->roots.stringClass, sizeof(beacon_SourceCode_t), BeaconObjectKindPointers);
    sourceCode->name = importedName;
    sourceCode->text = importedString;
    sourceCode->textSize = beacon_encodeSmallInteger(stringSize);
    return sourceCode;
}

beacon_SourcePosition_t *beacon_sourcePosition_to(beacon_context_t *context, beacon_SourcePosition_t *start, beacon_SourcePosition_t *end)
{
    beacon_SourcePosition_t *merged = beacon_allocateObjectWithBehavior(context->heap, context->roots.sourcePositionClass, sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
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
    beacon_SourcePosition_t *merged = beacon_allocateObjectWithBehavior(context->heap, context->roots.sourcePositionClass, sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
    merged->sourceCode  = start->sourceCode;
    merged->startIndex  = start->startIndex;
    merged->startLine   = start->startLine;
    merged->startColumn = start->startColumn;
    merged->endIndex    = end->startIndex;
    merged->endLine     = end->startLine;
    merged->endColumn   = end->startColumn;
    return merged;
}
