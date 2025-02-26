#include "Context.h"
#include "Exceptions.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "stb_truetype.h"

static inline int clampInteger(int x, int minX, int maxX)
{
    if(x < minX)
        return minX;
    else if(x > maxX)
        return maxX;
    return x;
}

static inline float clampFloat(float x, float minX, float maxX)
{
    if(x < minX)
        return minX;
    else if(x > maxX)
        return maxX;
    return x;
}

beacon_oop_t beacon_SolidRectangleRenderingElement_drawInForm(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.formClass);

    beacon_FormSolidRectangleRenderingElement_t *renderingElement = (beacon_FormSolidRectangleRenderingElement_t*)receiver;
    beacon_Form_t *form = (beacon_Form_t*)arguments[0];

    int formWidth = beacon_decodeSmallInteger(form->width);
    int formHeight = beacon_decodeSmallInteger(form->height);
    int formPitch = beacon_decodeSmallInteger(form->pitch);

    beacon_Rectangle_t *rectangle = renderingElement->super.rectangle;

    float minX = beacon_decodeNumber(rectangle->origin->x);
    float minY = beacon_decodeNumber(rectangle->origin->y);
    float maxX = beacon_decodeNumber(rectangle->corner->x);
    float maxY = beacon_decodeNumber(rectangle->corner->y);

    float r = clampFloat(beacon_decodeNumber(renderingElement->color->r), 0, 1);
    float g = clampFloat(beacon_decodeNumber(renderingElement->color->g), 0, 1);
    float b = clampFloat(beacon_decodeNumber(renderingElement->color->b), 0, 1);
    float a = clampFloat(beacon_decodeNumber(renderingElement->color->a), 0, 1);

    int br = (int)(r *255);
    int bg = (int)(g *255);
    int bb = (int)(b *255);
    int ba = (int)(a *255);

    uint32_t encodedColor = br | (bg << 8) | (bb << 16) | (ba << 24);
    
    float bor = clampFloat(beacon_decodeNumber(renderingElement->borderColor->r), 0, 1);
    float bog = clampFloat(beacon_decodeNumber(renderingElement->borderColor->g), 0, 1);
    float bob = clampFloat(beacon_decodeNumber(renderingElement->borderColor->b), 0, 1);
    float boa = clampFloat(beacon_decodeNumber(renderingElement->borderColor->a), 0, 1);

    int drawMinX = floor(clampFloat(minX, 0, formWidth) + 0.5);
    int drawMinY = floor(clampFloat(minY, 0, formHeight) + 0.5);
    int drawMaxX = floor(clampFloat(maxX, 0, formWidth) + 0.5);
    int drawMaxY = floor(clampFloat(maxY, 0, formHeight) + 0.5);

    int drawWidth = drawMaxX - drawMinX;
    int drawHeight = drawMaxY - drawMinY;

    uint8_t *destinationRow = form->bits->elements + drawMinY*formPitch;
    for(int y = 0; y < drawHeight; ++y)
    {
        uint32_t *destinationRowPixel = (uint32_t*)destinationRow;
        for(int x = 0; x < drawWidth; ++x)
        {
            destinationRowPixel[x + drawMinX] = encodedColor;
        }
        destinationRow += formPitch;
    }
    return receiver;
}

void beacon_TextRenderingElement_drawCharacterInForm(beacon_context_t *context,
        beacon_Form_t *atlasForm, int atlasMaX, int atlasMaxY, int atlasMinX, int atlasMinY,
        beacon_Form_t *targetForm, int targetMinX, int targetMinY, int targetMaxX, int targetMaxY,
        beacon_Color_t *color)
{
    BeaconAssert(context, beacon_decodeSmallInteger(atlasForm->depth) == 8);
    BeaconAssert(context, beacon_decodeSmallInteger(targetForm->depth) == 32);
 
    float r = clampFloat(beacon_decodeNumber(color->r), 0, 1);
    float g = clampFloat(beacon_decodeNumber(color->g), 0, 1);
    float b = clampFloat(beacon_decodeNumber(color->b), 0, 1);
    float a = clampFloat(beacon_decodeNumber(color->a), 0, 1);

    int br = (int)(r *255);
    int bg = (int)(g *255);
    int bb = (int)(b *255);
    int ba = (int)(a *255);

    int sourcePitch = beacon_decodeSmallInteger(atlasForm->pitch);

    int targetWidth = beacon_decodeSmallInteger(targetForm->width);
    int targetHeight = beacon_decodeSmallInteger(targetForm->height);
    int targetPitch = beacon_decodeSmallInteger(targetForm->pitch); 
    
    uint8_t *sourcePixels = atlasForm->bits->elements;
    uint8_t *destPixels = targetForm->bits->elements;

    uint8_t *sourceRow = sourcePixels + sourcePitch * atlasMinY;
    uint8_t *destRow = destPixels + targetPitch * targetMinY;
    for(int y = targetMinY; y < targetMaxY; ++y)
    {
        uint32_t *destRow32 = (uint32_t*)destRow;
        for(int x = targetMinX; x < targetMaxX; ++x)
        {
            if(0 <= x && x < targetWidth && 
               0 <= y && y < targetHeight)
            {
                uint8_t alpha = sourceRow[x];
                // TODO: perform proper alpha blending
                if(alpha > 0.1)
                {
                    destRow32[x] = 0x0;
                }
            }
        }
        sourceRow += sourcePitch;
        destRow += targetPitch;
    }
}

beacon_oop_t beacon_TextRenderingElement_drawInForm(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.formClass);
    beacon_FormTextRenderingElement_t *renderingElement = (beacon_FormTextRenderingElement_t*)receiver;
    beacon_Form_t *form = (beacon_Form_t*)arguments[0];
    beacon_FontFace_t *fontFace = renderingElement->fontFace;
    if(!renderingElement->fontFace)
        return receiver;
    
    float rectMinX = beacon_decodeNumber(renderingElement->super.rectangle->origin->x);
    float rectMinY = beacon_decodeNumber(renderingElement->super.rectangle->origin->y);
    float rectMaxX = beacon_decodeNumber(renderingElement->super.rectangle->corner->x);
    float rectMaxY = beacon_decodeNumber(renderingElement->super.rectangle->corner->y);

    int formWidth = beacon_decodeSmallInteger(form->width);
    int formHeight = beacon_decodeSmallInteger(form->height);

    int atlasFormWidth = beacon_decodeSmallInteger(fontFace->atlasForm->width);
    int atlasFormHeight = beacon_decodeSmallInteger(fontFace->atlasForm->height);

    float baselineX = rectMinX;
    float baselineY = rectMinY + (rectMaxY - rectMinY) * 0.5f ;
    size_t stringSize = renderingElement->text->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < stringSize; ++i)
    {
        char c = renderingElement->text->data[i];
        if(c < ' ')
            continue;

        stbtt_aligned_quad quadToDraw = {};
        stbtt_GetBakedQuad((stbtt_bakedchar*)fontFace->charData->elements, formWidth, formHeight, c - 31, &baselineX, &baselineY, &quadToDraw, true);

        beacon_TextRenderingElement_drawCharacterInForm(context,
                fontFace->atlasForm, atlasFormWidth*quadToDraw.s0, atlasFormHeight+quadToDraw.t0, atlasFormWidth*quadToDraw.s1, atlasFormHeight*quadToDraw.t1,
                form, quadToDraw.x0, quadToDraw.y0, quadToDraw.x1, quadToDraw.y1,
                renderingElement->color);
    }

    return receiver;
}

void beacon_context_registerFormRenderingPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.formSolidRectangleRenderingElementClass, "drawInForm:", 1, beacon_SolidRectangleRenderingElement_drawInForm);
    beacon_addPrimitiveToClass(context, context->classes.formTextRenderingElementClass, "drawInForm:", 1, beacon_TextRenderingElement_drawInForm);
}
