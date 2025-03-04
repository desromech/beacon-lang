#include "Context.h"
#include "Exceptions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION

typedef struct BeaconGlyphRectange_s
{
    float minX, minY;
    float maxX, maxY;
} BeaconGlyphRectange_t;

void glyphRectangle_addPoint(BeaconGlyphRectange_t *rectangle, int x, int y)
{
    if(x < rectangle->minX) rectangle->minX = x;
    if(x > rectangle->maxX) rectangle->maxX = x;
    if(y < rectangle->minY) rectangle->minY = y;
    if(y > rectangle->maxY) rectangle->maxY = y;
}

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
        free(fileNameCString);
        fprintf(stderr, "Failed to open font file.");
        return 0;
    }

    fseek(fontFile, 0, SEEK_END);
    size_t fontFileSize = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);

    beacon_ByteArray_t *fontData = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, sizeof(beacon_ByteArray_t) + fontFileSize, BeaconObjectKindBytes);
    fread(fontData->elements, fontFileSize, 1, fontFile);
    fclose(fontFile);

    free(fileNameCString);

    beacon_Font_t *font = beacon_allocateObjectWithBehavior(context->heap, context->classes.fontClass, sizeof(beacon_Font_t), BeaconObjectKindPointers);
    font->rawData = fontData;
    return (beacon_oop_t)font;
}

beacon_oop_t beacon_Font_createFaceWithHeight(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_Font_t *font = (beacon_Font_t *)receiver;
    int fontHeight = beacon_decodeSmallInteger(arguments[0]);
    size_t bitmapWidth = fontHeight < 20 ? 256 : 512;
    size_t bitmapHeight = fontHeight < 30 ? 256 : 512;
    size_t bitmapDepth = 8;
    size_t bitmapByteSize = bitmapWidth*bitmapHeight*(bitmapDepth / 8);
    beacon_ByteArray_t *bitmapData = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, sizeof(beacon_ByteArray_t) + bitmapByteSize, BeaconObjectKindBytes); 

    beacon_ByteArray_t *charData = beacon_allocateObjectWithBehavior(context->heap, context->classes.byteArrayClass, sizeof(beacon_ByteArray_t) + sizeof(stbtt_bakedchar)*(256-31), BeaconObjectKindBytes); 
    stbtt_bakedchar *bakedChar = (stbtt_bakedchar *)charData->elements;
    
    stbtt_BakeFontBitmap(font->rawData->elements, 0, fontHeight, bitmapData->elements, bitmapWidth, bitmapHeight, 31, 256 - 31, bakedChar);

    beacon_Form_t *bitmapForm = beacon_allocateObjectWithBehavior(context->heap, context->classes.formClass, sizeof(beacon_Form_t), BeaconObjectKindPointers); 
    bitmapForm->width = beacon_encodeSmallInteger(bitmapWidth);
    bitmapForm->height = beacon_encodeSmallInteger(bitmapHeight);
    bitmapForm->pitch = beacon_encodeSmallInteger(bitmapWidth);
    bitmapForm->depth = beacon_encodeSmallInteger(bitmapDepth);
    bitmapForm->bits = bitmapData;

    beacon_FontFace_t *fontFace = beacon_allocateObjectWithBehavior(context->heap, context->classes.fontFaceClass, sizeof(beacon_FontFace_t), BeaconObjectKindPointers);
    fontFace->charData = charData;
    fontFace->height = beacon_encodeSmallInteger(fontHeight);
    fontFace->atlasForm = bitmapForm;

    float ascent;
    float descent;
    float linegap;
    stbtt_GetScaledFontVMetrics(font->rawData->elements, 0, fontHeight, &ascent, &descent, &linegap);
    fontFace->ascent = beacon_encodeSmallFloat(ascent);
    fontFace->descent = beacon_encodeSmallFloat(descent);
    fontFace->linegap = beacon_encodeSmallFloat(linegap);


    //char fileNameBuffer[64];
    //snprintf(fileNameBuffer, sizeof(fileNameBuffer), "atlas-%d.data", fontHeight);
    //FILE *f = fopen(fileNameBuffer, "wb");
    //fwrite(bitmapData->elements, bitmapByteSize, 1, f);
    //fclose(f);
    return (beacon_oop_t)fontFace;
}

beacon_oop_t beacon_FontFace_measureTextExtent(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_FontFace_t *fontFace = (beacon_FontFace_t *)receiver;
    beacon_String_t *string = (beacon_String_t*)arguments[0];
    size_t stringSize = string->super.super.super.super.super.header.slotCount;

    size_t height = 0;
    size_t width = 0;
    beacon_Form_t *atlasForm = fontFace->atlasForm;
    int formWidth = beacon_decodeSmallInteger(atlasForm->width);
    int formHeight = beacon_decodeSmallInteger(atlasForm->height);

    // TODO: Decode UTF8
    BeaconGlyphRectange_t rectangle = {};
    float baselineX = 0;
    float baselineY = 0;
    for(size_t i = 0; i < stringSize; ++i)
    {
        char c = string->data[i];
        if(c <= ' ')
        {
            continue;
        }

        stbtt_aligned_quad quadToDraw = {};
        stbtt_GetBakedQuad((stbtt_bakedchar*)fontFace->charData->elements, formWidth, formHeight, c - 31, &baselineX, &baselineY, &quadToDraw, true);

        glyphRectangle_addPoint(&rectangle, quadToDraw.x0, quadToDraw.y0);
        glyphRectangle_addPoint(&rectangle, quadToDraw.x1, quadToDraw.y1);
        glyphRectangle_addPoint(&rectangle, baselineX, baselineY);

    }

    beacon_Point_t *extent = beacon_allocateObjectWithBehavior(context->heap, context->classes.pointClass, sizeof(beacon_Point_t), BeaconObjectKindPointers);
    extent->x = beacon_encodeSmallInteger(rectangle.maxX - rectangle.minX);
    extent->y = beacon_encodeSmallInteger(rectangle.maxY - rectangle.minY);
    return (beacon_oop_t)extent;
}

beacon_oop_t beacon_FontFace_measureTextExtentUntil(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    beacon_FontFace_t *fontFace = (beacon_FontFace_t *)receiver;
    beacon_String_t *string = (beacon_String_t*)arguments[0];
    size_t untilColumn = beacon_decodeSmallInteger(arguments[1]);
    size_t stringSize = string->super.super.super.super.super.header.slotCount;

    size_t height = 0;
    size_t width = 0;
    beacon_Form_t *atlasForm = fontFace->atlasForm;
    int formWidth = beacon_decodeSmallInteger(atlasForm->width);
    int formHeight = beacon_decodeSmallInteger(atlasForm->height);

    // TODO: Decode UTF8
    BeaconGlyphRectange_t rectangle = {};
    float baselineX = 0;
    float baselineY = 0;
    for(size_t i = 0; i < stringSize && i < untilColumn; ++i)
    {
        char c = string->data[i];
        if(c < ' ')
            continue;

        stbtt_aligned_quad quadToDraw = {};
        stbtt_GetBakedQuad((stbtt_bakedchar*)fontFace->charData->elements, formWidth, formHeight, c - 31, &baselineX, &baselineY, &quadToDraw, true);

        glyphRectangle_addPoint(&rectangle, quadToDraw.x0, quadToDraw.y0);
        glyphRectangle_addPoint(&rectangle, quadToDraw.x1, quadToDraw.y1);
        glyphRectangle_addPoint(&rectangle, baselineX, baselineY);

    }

    beacon_Point_t *extent = beacon_allocateObjectWithBehavior(context->heap, context->classes.pointClass, sizeof(beacon_Point_t), BeaconObjectKindPointers);
    extent->x = beacon_encodeSmallInteger(rectangle.maxX - rectangle.minX);
    extent->y = beacon_encodeSmallInteger(rectangle.maxY - rectangle.minY);
    return (beacon_oop_t)extent;
}

void beacon_context_registerFontFacePrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.fontClass), "loadFontFromFile:", 1, beacon_Font_LoadFontFromFile);
    beacon_addPrimitiveToClass(context, context->classes.fontClass, "createFaceWithHeight:", 1, beacon_Font_createFaceWithHeight);
    beacon_addPrimitiveToClass(context, context->classes.fontFaceClass, "measureTextExtent:", 1, beacon_FontFace_measureTextExtent);
    beacon_addPrimitiveToClass(context, context->classes.fontFaceClass, "measureTextExtent:until:", 2, beacon_FontFace_measureTextExtentUntil);
}
