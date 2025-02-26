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

    float colorR = clampFloat(beacon_decodeNumber(renderingElement->color->r), 0, 1);
    float colorG = clampFloat(beacon_decodeNumber(renderingElement->color->g), 0, 1);
    float colorB = clampFloat(beacon_decodeNumber(renderingElement->color->b), 0, 1);
    float colorA = clampFloat(beacon_decodeNumber(renderingElement->color->a), 0, 1);
    
    float borderColorR = clampFloat(beacon_decodeNumber(renderingElement->borderColor->r), 0, 1);
    float borderColorG = clampFloat(beacon_decodeNumber(renderingElement->borderColor->g), 0, 1);
    float borderColorB = clampFloat(beacon_decodeNumber(renderingElement->borderColor->b), 0, 1);
    float borderColorA = clampFloat(beacon_decodeNumber(renderingElement->borderColor->a), 0, 1);

    int drawMinX = floor(clampFloat(minX, 0, formWidth) + 0.5);
    int drawMinY = floor(clampFloat(minY, 0, formHeight) + 0.5);
    int drawMaxX = floor(clampFloat(maxX, 0, formWidth) + 0.5);
    int drawMaxY = floor(clampFloat(maxY, 0, formHeight) + 0.5);

    int borderSize = beacon_decodeNumber(renderingElement->super.borderSize);
    int interiorDrawMinX = drawMinX + borderSize;
    int interiorDrawMinY = drawMinY + borderSize;
    int interiorDrawMaxX = drawMaxX + borderSize;
    int interiorDrawMaxY = drawMaxY + borderSize;

    int drawWidth = drawMaxX - drawMinX;
    int drawHeight = drawMaxY - drawMinY;

    uint8_t *destinationRow = form->bits->elements + drawMinY*formPitch;
    for(int y = 0; y < drawHeight; ++y)
    {
        uint32_t *destinationRowPixel = (uint32_t*)destinationRow;
        for(int x = 0; x < drawWidth; ++x)
        {
            if(0 <= x && x < formWidth &&
               0 <= y && y < formHeight)
            {
                bool isInInterior = borderSize <= 0 ||
                    (interiorDrawMinX <= x && x < interiorDrawMaxX &&
                    interiorDrawMinY <= y && y < interiorDrawMaxY);

                bool isInBorder = !isInInterior; 
                float colorR = clampFloat(beacon_decodeNumber(renderingElement->color->r), 0, 1);
                float colorG = clampFloat(beacon_decodeNumber(renderingElement->color->g), 0, 1);
                float colorB = clampFloat(beacon_decodeNumber(renderingElement->color->b), 0, 1);
                float colorA = clampFloat(beacon_decodeNumber(renderingElement->color->a), 0, 1);

                float sourceR = isInBorder ? borderColorR : colorR;
                float sourceG = isInBorder ? borderColorG : colorG;
                float sourceB = isInBorder ? borderColorB : colorB;
                float sourceA = isInBorder ? borderColorA : colorA;
            
                uint32_t destinationPixel = destinationRowPixel[x + drawMinX];
                float destB = (destinationPixel & 0xFF)/255.0;
                float destG = ((destinationPixel >> 8) & 0xFF)/255.0;
                float destR = ((destinationPixel >> 16) & 0xFF)/255.0;
                float destA = ((destinationPixel >> 24) & 0xFF)/255.0;

                float pixelR = sourceR + destR*(1.0f - sourceA);
                float pixelG = sourceG + destG*(1.0f - sourceA);
                float pixelB = sourceB + destB*(1.0f - sourceA);
                float pixelA = sourceA + destA*(1.0f - sourceA);

                uint8_t blendR = (uint8_t)(pixelR*255);
                uint8_t blendG = (uint8_t)(pixelG*255);
                uint8_t blendB = (uint8_t)(pixelB*255);
                uint8_t blendA = (uint8_t)(pixelA*255);

                destinationRowPixel[x + drawMinX] = blendB | (blendG << 8) | (blendR << 16) | (blendA << 24);

            }
        }
        destinationRow += formPitch;
    }
    return receiver;
}

void beacon_TextRenderingElement_drawCharacterInForm(beacon_context_t *context,
        beacon_Form_t *atlasForm, int atlasMinX, int atlasMinY, int atlasMaxX, int atlasMaxY,
        beacon_Form_t *targetForm, int targetMinX, int targetMinY, int targetMaxX, int targetMaxY,
        beacon_Color_t *color)
{
    BeaconAssert(context, beacon_decodeSmallInteger(atlasForm->depth) == 8);
    BeaconAssert(context, beacon_decodeSmallInteger(targetForm->depth) == 32);
 
    float r = clampFloat(beacon_decodeNumber(color->r), 0, 1);
    float g = clampFloat(beacon_decodeNumber(color->g), 0, 1);
    float b = clampFloat(beacon_decodeNumber(color->b), 0, 1);
    float a = clampFloat(beacon_decodeNumber(color->a), 0, 1);
    r *= a;
    g *= a;
    b *= a;

    int sourcePitch = beacon_decodeSmallInteger(atlasForm->pitch);

    int targetWidth = beacon_decodeSmallInteger(targetForm->width);
    int targetHeight = beacon_decodeSmallInteger(targetForm->height);
    int targetPitch = beacon_decodeSmallInteger(targetForm->pitch); 
    
    int blitWidth = atlasMaxX - atlasMinX;
    int blitHeight = atlasMaxY - atlasMinY;

    uint8_t *sourcePixels = atlasForm->bits->elements;
    uint8_t *destPixels = targetForm->bits->elements;

    uint8_t *sourceRow = sourcePixels + sourcePitch * atlasMinY;
    uint8_t *destRow = destPixels + targetPitch * targetMinY;
    for(int y = 0; y < blitHeight; ++y)
    {
        int sourceY = atlasMinY + y;
        int destinationY = targetMinY + y;
         
        uint32_t *destRow32 = (uint32_t*)destRow;
        for(int x = 0; x < blitWidth; ++x)
        {
            int sourceX = atlasMinX + x;
            int destinationX = targetMinX + x;
    
            if(0 <= destinationX && destinationX < targetWidth && 
               0 <= destinationY && destinationY < targetHeight)
            {
                uint8_t alpha = sourceRow[sourceX];
                float fontAlpha = alpha/255.0f;
                float sourceR = r*fontAlpha;
                float sourceG = g*fontAlpha;
                float sourceB = b*fontAlpha;
                float sourceA = a*fontAlpha;

                uint32_t destinationPixel = destRow32[destinationX];
                float destB = (destinationPixel & 0xFF)/255.0;
                float destG = ((destinationPixel >> 8) & 0xFF)/255.0;
                float destR = ((destinationPixel >> 16) & 0xFF)/255.0;
                float destA = ((destinationPixel >> 24) & 0xFF)/255.0;

                float pixelR = sourceR + destR*(1.0f - sourceA);
                float pixelG = sourceG + destG*(1.0f - sourceA);
                float pixelB = sourceB + destB*(1.0f - sourceA);
                float pixelA = sourceA + destA*(1.0f - sourceA);

                uint8_t blendR = (uint8_t)(pixelR*255);
                uint8_t blendG = (uint8_t)(pixelG*255);
                uint8_t blendB = (uint8_t)(pixelB*255);
                uint8_t blendA = (uint8_t)(pixelA*255);

                destRow32[destinationX] = blendB | (blendG << 8) | (blendR << 16) | (blendA << 24);
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

    float baselineX = rectMinX;
    float baselineY = rectMinY + (rectMaxY - rectMinY) * 0.5f ;
    size_t stringSize = renderingElement->text->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < stringSize; ++i)
    {
        char c = renderingElement->text->data[i];
        if(c < ' ')
            continue;

        stbtt_aligned_quad quadToDraw = {};
        stbtt_GetBakedQuad((stbtt_bakedchar*)fontFace->charData->elements, 1, 1, c - 31, &baselineX, &baselineY, &quadToDraw, true);

        beacon_TextRenderingElement_drawCharacterInForm(context,
                fontFace->atlasForm, quadToDraw.s0, quadToDraw.t0, quadToDraw.s1, quadToDraw.t1,
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

