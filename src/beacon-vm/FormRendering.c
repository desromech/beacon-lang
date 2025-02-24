#include "Context.h"
#include "Exceptions.h"
#include <stdlib.h>
#include <stdio.h>

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

    float r = beacon_decodeNumber(renderingElement->color->r);
    float g = beacon_decodeNumber(renderingElement->color->g);
    float b = beacon_decodeNumber(renderingElement->color->b);
    float a = beacon_decodeNumber(renderingElement->color->a);

    float br = beacon_decodeNumber(renderingElement->borderColor->r);
    float bg = beacon_decodeNumber(renderingElement->borderColor->g);
    float bb = beacon_decodeNumber(renderingElement->borderColor->b);
    float ba = beacon_decodeNumber(renderingElement->borderColor->a);

    printf("TODO: beacon_SolidRectangleRenderingElement_drawInForm\n");
    return receiver;
}

void beacon_context_registerFormRenderingPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.formSolidRectangleRenderingElementClass, "drawInForm:", 1, beacon_SolidRectangleRenderingElement_drawInForm);
}
