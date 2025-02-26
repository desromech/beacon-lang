#include "Context.h"
#include "Exceptions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

beacon_oop_t beacon_Font_LoadFontFromFile(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    
    beacon_String_t *fileName = (beacon_String_t *)arguments[0];

    size_t fileNameSize = fileName->super.super.super.super.super.header.slotCount;
    char *fileNameCString = calloc(1, fileNameSize + 1);
    memcpy(fileNameCString, fileName->data, fileNameSize);

    FILE *fontFile = fopen(fileNameCString, "rb");
    if(!fontFile)
    {
        fprintf(stderr, "Failed to open font file.");
        return 0;
    }

    fseek(fontFile, 0, SEEK_END);
    size_t fontFileSize = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);

    beacon_ByteArray_t *fontData = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, fontFileSize, BeaconObjectKindBytes);
    fread(fontData->elements, fontFileSize, 1, fontFile);
    fclose(fontFile);

    beacon_Font_t *font = beacon_allocateObjectWithBehavior(context->heap, context->classes.fontClass, sizeof(beacon_Font_t), BeaconObjectKindPointers);
    font->rawData = fontData;
    return (beacon_oop_t)font;
}

beacon_oop_t beacon_Font_createFaceWithHeightSize(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_Font_t *font = (beacon_Font_t *)receiver;
    int fontHeight = beacon_decodeSmallInteger(arguments[0]);
    
    return receiver;
}

void beacon_context_registerFontFacePrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.fontClass), "loadFontFromFile:", 1, beacon_Font_LoadFontFromFile);
    beacon_addPrimitiveToClass(context, context->classes.fontClass, "createFaceWithHeightSize:", 1, beacon_Font_createFaceWithHeightSize);
}
