#include "ObjectModel.h"
#include "Context.h"
#include "Exceptions.h"
#include <math.h>

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

static beacon_oop_t beacon_Vector2_dot(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector2_t *leftVector = (beacon_Vector2_t *)receiver;
    beacon_Vector2_t *rightVector = (beacon_Vector2_t *)arguments[0];
    double result = leftVector->x*rightVector->x + leftVector->y*rightVector->y;
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Vector2_length(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector2_t *vector = (beacon_Vector2_t *)receiver;
    double result = sqrt(vector->x*vector->x + vector->y*vector->y);
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Vector2_add(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector2_t *leftVector = (beacon_Vector2_t *)receiver;
    beacon_Vector2_t *rightVector = (beacon_Vector2_t *)arguments[0];
    beacon_Vector2_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x + rightVector->x;
    resultVector->y = leftVector->y + rightVector->y;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector2_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector2_t *leftVector = (beacon_Vector2_t *)receiver;
    beacon_Vector2_t *rightVector = (beacon_Vector2_t *)arguments[0];
    beacon_Vector2_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x - rightVector->x;
    resultVector->y = leftVector->y - rightVector->y;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector2_times(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector2_t *leftVector = (beacon_Vector2_t *)receiver;
    beacon_Vector2_t *rightVector = (beacon_Vector2_t *)arguments[0];
    beacon_Vector2_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x * rightVector->x;
    resultVector->y = leftVector->y * rightVector->y;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector2_divide(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector2_t *leftVector = (beacon_Vector2_t *)receiver;
    beacon_Vector2_t *rightVector = (beacon_Vector2_t *)arguments[0];
    beacon_Vector2_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x / rightVector->x;
    resultVector->y = leftVector->y / rightVector->y;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector3_constructWithXYZ(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 3);
    double x = beacon_decodeNumberAsDouble(context, arguments[0]);
    double y = beacon_decodeNumberAsDouble(context, arguments[1]);
    double z = beacon_decodeNumberAsDouble(context, arguments[2]);
    beacon_Vector3_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);
    vector->x = x;
    vector->y = y;
    vector->z = z;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Vector3_x(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector3_t *vector = (beacon_Vector3_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->x);
}

static beacon_oop_t beacon_Vector3_y(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector3_t *vector = (beacon_Vector3_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->y);
}

static beacon_oop_t beacon_Vector3_z(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector3_t *vector = (beacon_Vector3_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->z);
}

static beacon_oop_t beacon_Vector3_dot(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector3_t *leftVector = (beacon_Vector3_t *)receiver;
    beacon_Vector3_t *rightVector = (beacon_Vector3_t *)arguments[0];
    double result = leftVector->x*rightVector->x + leftVector->y*rightVector->y + leftVector->z*rightVector->z;
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Vector3_length(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector3_t *vector = (beacon_Vector3_t *)receiver;
    double result = sqrt(vector->x*vector->x + vector->y*vector->y + vector->z*vector->z);
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Vector3_add(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector3_t *leftVector = (beacon_Vector3_t *)receiver;
    beacon_Vector3_t *rightVector = (beacon_Vector3_t *)arguments[0];
    beacon_Vector3_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x + rightVector->x;
    resultVector->y = leftVector->y + rightVector->y;
    resultVector->z = leftVector->z + rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector3_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector3_t *leftVector = (beacon_Vector3_t *)receiver;
    beacon_Vector3_t *rightVector = (beacon_Vector3_t *)arguments[0];
    beacon_Vector3_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x - rightVector->x;
    resultVector->y = leftVector->y - rightVector->y;
    resultVector->z = leftVector->z - rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector3_times(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector3_t *leftVector = (beacon_Vector3_t *)receiver;
    beacon_Vector3_t *rightVector = (beacon_Vector3_t *)arguments[0];
    beacon_Vector3_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x * rightVector->x;
    resultVector->y = leftVector->y * rightVector->y;
    resultVector->z = leftVector->z * rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector3_divide(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector3_t *leftVector = (beacon_Vector3_t *)receiver;
    beacon_Vector3_t *rightVector = (beacon_Vector3_t *)arguments[0];
    beacon_Vector3_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x / rightVector->x;
    resultVector->y = leftVector->y / rightVector->y;
    resultVector->z = leftVector->z / rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector4_constructWithXYZW(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 4);
    double x = beacon_decodeNumberAsDouble(context, arguments[0]);
    double y = beacon_decodeNumberAsDouble(context, arguments[1]);
    double z = beacon_decodeNumberAsDouble(context, arguments[2]);
    double w = beacon_decodeNumberAsDouble(context, arguments[3]);
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);
    vector->x = x;
    vector->y = y;
    vector->z = z;
    vector->w = w;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Vector4_x(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector4_t *vector = (beacon_Vector4_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->x);
}

static beacon_oop_t beacon_Vector4_y(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector4_t *vector = (beacon_Vector4_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->y);
}

static beacon_oop_t beacon_Vector4_z(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector4_t *vector = (beacon_Vector4_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->z);
}

static beacon_oop_t beacon_Vector4_w(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector4_t *vector = (beacon_Vector4_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->w);
}

static beacon_oop_t beacon_Vector4_dot(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector4_t *leftVector = (beacon_Vector4_t *)receiver;
    beacon_Vector4_t *rightVector = (beacon_Vector4_t *)arguments[0];
    double result = leftVector->x*rightVector->x + leftVector->y*rightVector->y + leftVector->z*rightVector->z + leftVector->w*rightVector->w;
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Vector4_length(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector4_t *vector = (beacon_Vector4_t *)receiver;
    double result = sqrt(vector->x*vector->x + vector->y*vector->y + vector->z*vector->z + vector->w*vector->w);
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Vector4_add(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector4_t *leftVector = (beacon_Vector4_t *)receiver;
    beacon_Vector4_t *rightVector = (beacon_Vector4_t *)arguments[0];
    beacon_Vector4_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x + rightVector->x;
    resultVector->y = leftVector->y + rightVector->y;
    resultVector->z = leftVector->z + rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector4_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector4_t *leftVector = (beacon_Vector4_t *)receiver;
    beacon_Vector4_t *rightVector = (beacon_Vector4_t *)arguments[0];
    beacon_Vector4_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x - rightVector->x;
    resultVector->y = leftVector->y - rightVector->y;
    resultVector->z = leftVector->z - rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector4_times(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector4_t *leftVector = (beacon_Vector4_t *)receiver;
    beacon_Vector4_t *rightVector = (beacon_Vector4_t *)arguments[0];
    beacon_Vector4_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x * rightVector->x;
    resultVector->y = leftVector->y * rightVector->y;
    resultVector->z = leftVector->z * rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector4_divide(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Vector4_t *leftVector = (beacon_Vector4_t *)receiver;
    beacon_Vector4_t *rightVector = (beacon_Vector4_t *)arguments[0];
    beacon_Vector4_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x / rightVector->x;
    resultVector->y = leftVector->y / rightVector->y;
    resultVector->z = leftVector->z / rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Complex_constructWithXY(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    double x = beacon_decodeNumberAsDouble(context, arguments[0]);
    double y = beacon_decodeNumberAsDouble(context, arguments[1]);
    beacon_Complex_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.complexClass, sizeof(beacon_Complex_t), BeaconObjectKindBytes);
    vector->x = x;
    vector->y = y;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Complex_x(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Complex_t *vector = (beacon_Complex_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->x);
}

static beacon_oop_t beacon_Complex_y(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Complex_t *vector = (beacon_Complex_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->y);
}

static beacon_oop_t beacon_Complex_dot(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Complex_t *leftVector = (beacon_Complex_t *)receiver;
    beacon_Complex_t *rightVector = (beacon_Complex_t *)arguments[0];
    double result = leftVector->x*rightVector->x + leftVector->y*rightVector->y;
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Complex_length(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Complex_t *vector = (beacon_Complex_t *)receiver;
    double result = sqrt(vector->x*vector->x + vector->y*vector->y);
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Complex_add(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Complex_t *leftVector = (beacon_Complex_t *)receiver;
    beacon_Complex_t *rightVector = (beacon_Complex_t *)arguments[0];
    beacon_Complex_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.complexClass, sizeof(beacon_Complex_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x + rightVector->x;
    resultVector->y = leftVector->y + rightVector->y;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Complex_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Complex_t *leftVector = (beacon_Complex_t *)receiver;
    beacon_Complex_t *rightVector = (beacon_Complex_t *)arguments[0];
    beacon_Complex_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.complexClass, sizeof(beacon_Complex_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x - rightVector->x;
    resultVector->y = leftVector->y - rightVector->y;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Quaternion_constructWithXYZW(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 4);
    double x = beacon_decodeNumberAsDouble(context, arguments[0]);
    double y = beacon_decodeNumberAsDouble(context, arguments[1]);
    double z = beacon_decodeNumberAsDouble(context, arguments[2]);
    double w = beacon_decodeNumberAsDouble(context, arguments[3]);
    beacon_Quaternion_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.quaternionClass, sizeof(beacon_Quaternion_t), BeaconObjectKindBytes);
    vector->x = x;
    vector->y = y;
    vector->z = z;
    vector->w = w;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Quaternion_x(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Quaternion_t *vector = (beacon_Quaternion_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->x);
}

static beacon_oop_t beacon_Quaternion_y(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Quaternion_t *vector = (beacon_Quaternion_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->y);
}

static beacon_oop_t beacon_Quaternion_z(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Quaternion_t *vector = (beacon_Quaternion_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->z);
}

static beacon_oop_t beacon_Quaternion_w(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Quaternion_t *vector = (beacon_Quaternion_t *)receiver;
    return beacon_encodeDoubleAsNumber(context, vector->w);
}

static beacon_oop_t beacon_Quaternion_dot(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Quaternion_t *leftVector = (beacon_Quaternion_t *)receiver;
    beacon_Quaternion_t *rightVector = (beacon_Quaternion_t *)arguments[0];
    double result = leftVector->x*rightVector->x + leftVector->y*rightVector->y + leftVector->z*rightVector->z + leftVector->w*rightVector->w;
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Quaternion_length(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Quaternion_t *vector = (beacon_Quaternion_t *)receiver;
    double result = sqrt(vector->x*vector->x + vector->y*vector->y + vector->z*vector->z + vector->w*vector->w);
    return beacon_encodeDoubleAsNumber(context, result);
}

static beacon_oop_t beacon_Quaternion_add(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Quaternion_t *leftVector = (beacon_Quaternion_t *)receiver;
    beacon_Quaternion_t *rightVector = (beacon_Quaternion_t *)arguments[0];
    beacon_Quaternion_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.quaternionClass, sizeof(beacon_Quaternion_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x + rightVector->x;
    resultVector->y = leftVector->y + rightVector->y;
    resultVector->z = leftVector->z + rightVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Quaternion_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Quaternion_t *leftVector = (beacon_Quaternion_t *)receiver;
    beacon_Quaternion_t *rightVector = (beacon_Quaternion_t *)arguments[0];
    beacon_Quaternion_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.quaternionClass, sizeof(beacon_Quaternion_t), BeaconObjectKindBytes);
    resultVector->x = leftVector->x - rightVector->x;
    resultVector->y = leftVector->y - rightVector->y;
    resultVector->z = leftVector->z - rightVector->z;
    return (beacon_oop_t)resultVector;
}

void beacon_context_registerLinearAlgebraPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.vector2Class), "x:y:", 2, beacon_Vector2_constructWithXY);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "x", 0, beacon_Vector2_x);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "y", 0, beacon_Vector2_y);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "dot:", 1, beacon_Vector2_dot);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "length", 0, beacon_Vector2_length);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "+", 1, beacon_Vector2_add);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "-", 1, beacon_Vector2_minus);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "*", 1, beacon_Vector2_times);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "/", 1, beacon_Vector2_divide);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.vector3Class), "x:y:z:", 3, beacon_Vector3_constructWithXYZ);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "x", 0, beacon_Vector3_x);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "y", 0, beacon_Vector3_y);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "z", 0, beacon_Vector3_z);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "dot:", 1, beacon_Vector3_dot);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "length", 0, beacon_Vector3_length);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "+", 1, beacon_Vector3_add);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "-", 1, beacon_Vector3_minus);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "*", 1, beacon_Vector3_times);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "/", 1, beacon_Vector3_divide);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.vector4Class), "x:y:z:w:", 4, beacon_Vector4_constructWithXYZW);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "x", 0, beacon_Vector4_x);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "y", 0, beacon_Vector4_y);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "z", 0, beacon_Vector4_z);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "w", 0, beacon_Vector4_w);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "dot:", 1, beacon_Vector4_dot);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "length", 0, beacon_Vector4_length);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "+", 1, beacon_Vector4_add);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "-", 1, beacon_Vector4_minus);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "*", 1, beacon_Vector4_times);
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "/", 1, beacon_Vector4_divide);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.complexClass), "x:y:", 2, beacon_Complex_constructWithXY);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.complexClass), "r:i:", 2, beacon_Complex_constructWithXY);
    beacon_addPrimitiveToClass(context, context->classes.complexClass, "x", 0, beacon_Complex_x);
    beacon_addPrimitiveToClass(context, context->classes.complexClass, "r", 0, beacon_Complex_x);
    beacon_addPrimitiveToClass(context, context->classes.complexClass, "y", 0, beacon_Complex_y);
    beacon_addPrimitiveToClass(context, context->classes.complexClass, "i", 0, beacon_Complex_y);
    beacon_addPrimitiveToClass(context, context->classes.complexClass, "dot:", 1, beacon_Complex_dot);
    beacon_addPrimitiveToClass(context, context->classes.complexClass, "length", 0, beacon_Complex_length);
    beacon_addPrimitiveToClass(context, context->classes.complexClass, "+", 1, beacon_Complex_add);
    beacon_addPrimitiveToClass(context, context->classes.complexClass, "-", 1, beacon_Complex_minus);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.quaternionClass), "x:y:z:w:", 4, beacon_Quaternion_constructWithXYZW);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.quaternionClass), "i:j:k:r:", 4, beacon_Quaternion_constructWithXYZW);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "x", 0, beacon_Quaternion_x);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "i", 0, beacon_Quaternion_x);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "y", 0, beacon_Quaternion_y);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "j", 0, beacon_Quaternion_y);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "z", 0, beacon_Quaternion_z);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "k", 0, beacon_Quaternion_z);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "w", 0, beacon_Quaternion_w);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "r", 0, beacon_Quaternion_w);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "dot:", 1, beacon_Quaternion_dot);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "length", 0, beacon_Quaternion_length);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "+", 1, beacon_Quaternion_add);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "-", 1, beacon_Quaternion_minus);
}
