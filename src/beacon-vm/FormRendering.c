#include "Context.h"
#include "Exceptions.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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

void beacon_context_registerFormRenderingPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.formSolidRectangleRenderingElementClass, "drawInForm:", 1, beacon_SolidRectangleRenderingElement_drawInForm);
}
