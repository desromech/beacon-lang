#include "ObjectModel.h"
#include "Context.h"
#include "Exceptions.h"

static beacon_oop_t beacon_Vector2_constructWithXY(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    double x = beacon_decodeNumberAsDouble(context, arguments[0]);
    double y = beacon_decodeNumberAsDouble(context, arguments[1]);
    beacon_Vector2_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);
    vector->x = x;
    vector->y = y;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Vector2_x(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector2_t *vector = (beacon_Vector2_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->x);
}

static beacon_oop_t beacon_Vector2_y(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector2_t *vector = (beacon_Vector2_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->y);
}

void beacon_context_registerLinearAlgebraPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.vector2Class), "x:y:", 2, beacon_Vector2_constructWithXY);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "x", 0, beacon_Vector2_x);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "y", 0, beacon_Vector2_y);
}
