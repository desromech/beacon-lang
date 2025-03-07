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

static beacon_oop_t beacon_Vector2_negated(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector2_t *operandVector = (beacon_Vector2_t *)receiver;
    beacon_Vector2_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);
    resultVector->x = -operandVector->x;
    resultVector->y = -operandVector->y;
    return (beacon_oop_t)resultVector;
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


static beacon_oop_t beacon_Vector3_asVector3(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    return receiver;
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

static beacon_oop_t beacon_Vector3_negated(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector3_t *operandVector = (beacon_Vector3_t *)receiver;
    beacon_Vector3_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);
    resultVector->x = -operandVector->x;
    resultVector->y = -operandVector->y;
    resultVector->z = -operandVector->z;
    return (beacon_oop_t)resultVector;
}

static beacon_oop_t beacon_Vector3_reciprocal(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector3_t *operandVector = (beacon_Vector3_t *)receiver;
    beacon_Vector3_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);
    resultVector->x = 1.0 / operandVector->x;
    resultVector->y = 1.0 / operandVector->y;
    resultVector->z = 1.0 / operandVector->z;
    return (beacon_oop_t)resultVector;
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

static beacon_oop_t beacon_Vector4_negated(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Vector4_t *operandVector = (beacon_Vector4_t *)receiver;
    beacon_Vector4_t *resultVector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);
    resultVector->x = -operandVector->x;
    resultVector->y = -operandVector->y;
    resultVector->z = -operandVector->z;
    resultVector->w = -operandVector->w;
    return (beacon_oop_t)resultVector;
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

static beacon_oop_t beacon_Quaternion_asMatrix3x3(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Quaternion_t *quat = (beacon_Quaternion_t *)receiver;
    beacon_Matrix3x3_t *mat = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix3x3Class, sizeof(beacon_Matrix3x3_t), BeaconObjectKindBytes);
    double i = quat->x;
    double j = quat->y;
    double k = quat->z;
    double r = quat->w;

    mat->m11 = 1.0 - (2.0*j*j) - (2.0*k*k);
    mat->m12 = (2.0*i*j) - (2.0*k*r);
    mat->m13 = (2.0*i*k) + (2.0*j*r);

    mat->m21 = (2.0*i*j) + (2.0*k*r);
    mat->m22 = 1.0 - (2.0*i*i) - (2.0*k*k);
    mat->m23 = (2.0*j*k) - (2.0*i*r);

    mat->m31 = (2.0*i*k) - (2.0*j*r);
    mat->m32 = (2.0*j*k) + (2.0*i*r);
    mat->m33 = 1.0 - (2.0*i*i) - (2.0*j*j);
    return (beacon_oop_t)mat;
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

static beacon_oop_t beacon_Matrix2x2_identity(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix2x2_t *matrix = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix2x2Class, sizeof(beacon_Matrix2x2_t), BeaconObjectKindBytes);
    matrix->m11 = 1;
    matrix->m22 = 1;
    return (beacon_oop_t)matrix;
}

static beacon_oop_t beacon_Matrix2x2_firstColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix2x2_t *matrix = (beacon_Matrix2x2_t*)receiver;
    beacon_Vector2_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);

    vector->x = matrix->m11;
    vector->y = matrix->m21;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix2x2_secondColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix2x2_t *matrix = (beacon_Matrix2x2_t*)receiver;
    beacon_Vector2_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);

    vector->x = matrix->m12;
    vector->y = matrix->m22;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix2x2_firstRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix2x2_t *matrix = (beacon_Matrix2x2_t*)receiver;
    beacon_Vector2_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);

    vector->x = matrix->m11;
    vector->y = matrix->m12;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix2x2_secondRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix2x2_t *matrix = (beacon_Matrix2x2_t*)receiver;
    beacon_Vector2_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector2Class, sizeof(beacon_Vector2_t), BeaconObjectKindBytes);

    vector->x = matrix->m21;
    vector->y = matrix->m22;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix2x2_add(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.matrix2x2Class);

    beacon_Matrix2x2_t *leftMatrix = (beacon_Matrix2x2_t *)receiver;
    beacon_Matrix2x2_t *rightMatrix = (beacon_Matrix2x2_t *)arguments[0];
    beacon_Matrix2x2_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix2x2Class, sizeof(beacon_Matrix2x2_t), BeaconObjectKindBytes);

    result->m11 = leftMatrix->m11 + rightMatrix->m11; result->m12 = leftMatrix->m12 + rightMatrix->m12;
    result->m21 = leftMatrix->m21 + rightMatrix->m21; result->m22 = leftMatrix->m22 + rightMatrix->m22;

    return (beacon_oop_t)result;
}

static beacon_oop_t beacon_Matrix2x2_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.matrix2x2Class);

    beacon_Matrix2x2_t *leftMatrix = (beacon_Matrix2x2_t *)receiver;
    beacon_Matrix2x2_t *rightMatrix = (beacon_Matrix2x2_t *)arguments[0];
    beacon_Matrix2x2_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix2x2Class, sizeof(beacon_Matrix2x2_t), BeaconObjectKindBytes);

    result->m11 = leftMatrix->m11 - rightMatrix->m11; result->m12 = leftMatrix->m12 - rightMatrix->m12;
    result->m21 = leftMatrix->m21 - rightMatrix->m21; result->m22 = leftMatrix->m22 - rightMatrix->m22;

    return (beacon_oop_t)result;
}


static beacon_oop_t beacon_Matrix3x3_identity(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix3x3_t *matrix = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix3x3Class, sizeof(beacon_Matrix3x3_t), BeaconObjectKindBytes);
    matrix->m11 = 1;
    matrix->m22 = 1;
    matrix->m33 = 1;
    return (beacon_oop_t)matrix;
}

static beacon_oop_t beacon_Matrix3x3_scale(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);

    beacon_Matrix3x3_t *matrix = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix3x3Class, sizeof(beacon_Matrix3x3_t), BeaconObjectKindBytes);
    if(beacon_isImmediate(arguments[0]))
    {
        double uniformScale = beacon_decodeSmallNumber(arguments[0]);
        matrix->m11 = uniformScale;
        matrix->m22 = uniformScale;
        matrix->m33 = uniformScale;    
        return (beacon_oop_t)matrix;
    }

    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.vector3Class);
    beacon_Vector3_t *vector = (beacon_Vector3_t *)arguments[0];
    matrix->m11 = vector->x;
    matrix->m22 = vector->y;
    matrix->m33 = vector->z;    
    return (beacon_oop_t)matrix;
}

static beacon_oop_t beacon_Matrix3x3_asMatrix3x3(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    return receiver;
}

static beacon_oop_t beacon_Matrix3x3_transpose(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix3x3_t *matrix = (beacon_Matrix3x3_t*)receiver;
    beacon_Matrix3x3_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix3x3Class, sizeof(beacon_Matrix3x3_t), BeaconObjectKindBytes);

    result->m11 = matrix->m11;
    result->m12 = matrix->m21;
    result->m13 = matrix->m31;

    result->m21 = matrix->m12;
    result->m22 = matrix->m22;
    result->m23 = matrix->m32;

    result->m31 = matrix->m13;
    result->m32 = matrix->m23;
    result->m33 = matrix->m33;

    return (beacon_oop_t)result;
}

static beacon_oop_t beacon_Matrix3x3_firstColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix3x3_t *matrix = (beacon_Matrix3x3_t*)receiver;
    beacon_Vector3_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);

    vector->x = matrix->m11;
    vector->y = matrix->m21;
    vector->z = matrix->m31;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix3x3_secondColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix3x3_t *matrix = (beacon_Matrix3x3_t*)receiver;
    beacon_Vector3_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);

    vector->x = matrix->m12;
    vector->y = matrix->m22;
    vector->z = matrix->m32;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix3x3_thirdColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix3x3_t *matrix = (beacon_Matrix3x3_t*)receiver;
    beacon_Vector3_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);

    vector->x = matrix->m13;
    vector->y = matrix->m23;
    vector->z = matrix->m33;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix3x3_firstRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix3x3_t *matrix = (beacon_Matrix3x3_t*)receiver;
    beacon_Vector3_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);

    vector->x = matrix->m11;
    vector->y = matrix->m12;
    vector->z = matrix->m13;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix3x3_secondRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix3x3_t *matrix = (beacon_Matrix3x3_t*)receiver;
    beacon_Vector3_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);

    vector->x = matrix->m21;
    vector->y = matrix->m22;
    vector->z = matrix->m23;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix3x3_thirdRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix3x3_t *matrix = (beacon_Matrix3x3_t*)receiver;
    beacon_Vector3_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);

    vector->x = matrix->m31;
    vector->y = matrix->m32;
    vector->z = matrix->m33;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix3x3_add(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.matrix3x3Class);

    beacon_Matrix3x3_t *leftMatrix  = (beacon_Matrix3x3_t *)receiver;
    beacon_Matrix3x3_t *rightMatrix = (beacon_Matrix3x3_t *)arguments[0];
    beacon_Matrix3x3_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix3x3Class, sizeof(beacon_Matrix3x3_t), BeaconObjectKindBytes);
    result->m11 = leftMatrix->m11 + rightMatrix->m11; result->m12 = leftMatrix->m12 + rightMatrix->m12; result->m13 = leftMatrix->m13 + rightMatrix->m13;
    result->m21 = leftMatrix->m21 + rightMatrix->m21; result->m22 = leftMatrix->m22 + rightMatrix->m22; result->m23 = leftMatrix->m23 + rightMatrix->m23;
    result->m31 = leftMatrix->m31 + rightMatrix->m31; result->m32 = leftMatrix->m32 + rightMatrix->m32; result->m33 = leftMatrix->m33 + rightMatrix->m33;

    return (beacon_oop_t)result;
}

static beacon_oop_t beacon_Matrix3x3_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.matrix3x3Class);

    beacon_Matrix3x3_t *leftMatrix  = (beacon_Matrix3x3_t *)receiver;
    beacon_Matrix3x3_t *rightMatrix = (beacon_Matrix3x3_t *)arguments[0];
    beacon_Matrix3x3_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix3x3Class, sizeof(beacon_Matrix3x3_t), BeaconObjectKindBytes);
    result->m11 = leftMatrix->m11 - rightMatrix->m11; result->m12 = leftMatrix->m12 - rightMatrix->m12; result->m13 = leftMatrix->m13 - rightMatrix->m13;
    result->m21 = leftMatrix->m21 - rightMatrix->m21; result->m22 = leftMatrix->m22 - rightMatrix->m22; result->m23 = leftMatrix->m23 - rightMatrix->m23;
    result->m31 = leftMatrix->m31 - rightMatrix->m31; result->m32 = leftMatrix->m32 - rightMatrix->m32; result->m33 = leftMatrix->m33 - rightMatrix->m33;

    return (beacon_oop_t)result;
}

static beacon_oop_t beacon_Matrix3x3_multiply(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    beacon_Behavior_t *argumentClass = beacon_getClass(context, arguments[0]);
    BeaconAssert(context, argumentClass == context->classes.matrix3x3Class || argumentClass == context->classes.vector3Class);

    if(argumentClass == context->classes.matrix3x3Class)
    {
        beacon_Matrix3x3_t *leftMatrix  = (beacon_Matrix3x3_t *)receiver;
        beacon_Matrix3x3_t *rightMatrix = (beacon_Matrix3x3_t *)arguments[0];
        beacon_Matrix3x3_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix3x3Class, sizeof(beacon_Matrix3x3_t), BeaconObjectKindBytes);
    
        result->m11 = leftMatrix->m11*rightMatrix->m11 + leftMatrix->m12*rightMatrix->m21 + leftMatrix->m13*rightMatrix->m31;
        result->m12 = leftMatrix->m11*rightMatrix->m12 + leftMatrix->m12*rightMatrix->m22 + leftMatrix->m13*rightMatrix->m32;
        result->m13 = leftMatrix->m11*rightMatrix->m13 + leftMatrix->m12*rightMatrix->m23 + leftMatrix->m13*rightMatrix->m33;

        result->m21 = leftMatrix->m21*rightMatrix->m11 + leftMatrix->m22*rightMatrix->m21 + leftMatrix->m23*rightMatrix->m31;
        result->m22 = leftMatrix->m21*rightMatrix->m12 + leftMatrix->m22*rightMatrix->m22 + leftMatrix->m23*rightMatrix->m32;
        result->m23 = leftMatrix->m21*rightMatrix->m13 + leftMatrix->m22*rightMatrix->m23 + leftMatrix->m23*rightMatrix->m33;

        result->m31 = leftMatrix->m31*rightMatrix->m11 + leftMatrix->m32*rightMatrix->m21 + leftMatrix->m33*rightMatrix->m31;
        result->m32 = leftMatrix->m31*rightMatrix->m12 + leftMatrix->m32*rightMatrix->m22 + leftMatrix->m33*rightMatrix->m32;
        result->m33 = leftMatrix->m31*rightMatrix->m13 + leftMatrix->m32*rightMatrix->m23 + leftMatrix->m33*rightMatrix->m33;

        return (beacon_oop_t)result;
    }
    else
    {
        beacon_Matrix3x3_t *leftMatrix  = (beacon_Matrix3x3_t *)receiver;
        beacon_Vector3_t *rightVector = (beacon_Vector3_t *)arguments[0];
        beacon_Vector3_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector3Class, sizeof(beacon_Vector3_t), BeaconObjectKindBytes);
        result->x = leftMatrix->m11 * rightVector->x + leftMatrix->m12 * rightVector->y + leftMatrix->m13 * rightVector->z;
        result->y = leftMatrix->m21 * rightVector->x + leftMatrix->m22 * rightVector->y + leftMatrix->m23 * rightVector->z;
        result->z = leftMatrix->m31 * rightVector->x + leftMatrix->m32 * rightVector->y + leftMatrix->m33 * rightVector->z;
        return (beacon_oop_t)result;
    }
}

static beacon_oop_t beacon_Matrix4x4_identity(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix4x4Class, sizeof(beacon_Matrix4x4_t), BeaconObjectKindBytes);
    matrix->m11 = 1;
    matrix->m22 = 1;
    matrix->m33 = 1;
    matrix->m44 = 1;
    return (beacon_oop_t)matrix;
}

static beacon_oop_t beacon_Matrix4x4_firstColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = (beacon_Matrix4x4_t*)receiver;
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);

    vector->x = matrix->m11;
    vector->y = matrix->m21;
    vector->z = matrix->m31;
    vector->w = matrix->m41;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix4x4_secondColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = (beacon_Matrix4x4_t*)receiver;
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);

    vector->x = matrix->m12;
    vector->y = matrix->m22;
    vector->z = matrix->m32;
    vector->w = matrix->m42;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix4x4_thirdColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = (beacon_Matrix4x4_t*)receiver;
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);

    vector->x = matrix->m13;
    vector->y = matrix->m23;
    vector->z = matrix->m33;
    vector->w = matrix->m43;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix4x4_fourthColumn(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = (beacon_Matrix4x4_t*)receiver;
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);

    vector->x = matrix->m14;
    vector->y = matrix->m24;
    vector->z = matrix->m34;
    vector->w = matrix->m44;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix4x4_firstRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = (beacon_Matrix4x4_t*)receiver;
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);

    vector->x = matrix->m11;
    vector->y = matrix->m12;
    vector->z = matrix->m13;
    vector->w = matrix->m14;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix4x4_secondRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = (beacon_Matrix4x4_t*)receiver;
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);

    vector->x = matrix->m21;
    vector->y = matrix->m22;
    vector->z = matrix->m23;
    vector->w = matrix->m24;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix4x4_thirdRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = (beacon_Matrix4x4_t*)receiver;
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);

    vector->x = matrix->m31;
    vector->y = matrix->m32;
    vector->z = matrix->m33;
    vector->w = matrix->m34;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix4x4_fourthRow(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);

    beacon_Matrix4x4_t *matrix = (beacon_Matrix4x4_t*)receiver;
    beacon_Vector4_t *vector = beacon_allocateObjectWithBehavior(context->heap, context->classes.vector4Class, sizeof(beacon_Vector4_t), BeaconObjectKindBytes);

    vector->x = matrix->m41;
    vector->y = matrix->m42;
    vector->z = matrix->m43;
    vector->w = matrix->m44;
    return (beacon_oop_t)vector;
}

static beacon_oop_t beacon_Matrix4x4_add(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.matrix4x4Class);

    beacon_Matrix4x4_t *leftMatrix  = (beacon_Matrix4x4_t *)receiver;
    beacon_Matrix4x4_t *rightMatrix = (beacon_Matrix4x4_t *)arguments[0];
    beacon_Matrix4x4_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix4x4Class, sizeof(beacon_Matrix4x4_t), BeaconObjectKindBytes);
    result->m11 = leftMatrix->m11 + rightMatrix->m11; result->m12 = leftMatrix->m12 + rightMatrix->m12; result->m13 = leftMatrix->m13 + rightMatrix->m13; result->m14 = leftMatrix->m14 + rightMatrix->m14;
    result->m21 = leftMatrix->m21 + rightMatrix->m21; result->m22 = leftMatrix->m22 + rightMatrix->m22; result->m23 = leftMatrix->m23 + rightMatrix->m23; result->m24 = leftMatrix->m24 + rightMatrix->m24;
    result->m31 = leftMatrix->m31 + rightMatrix->m31; result->m32 = leftMatrix->m32 + rightMatrix->m32; result->m33 = leftMatrix->m33 + rightMatrix->m33; result->m34 = leftMatrix->m34 + rightMatrix->m34; 
    result->m41 = leftMatrix->m41 + rightMatrix->m41; result->m42 = leftMatrix->m42 + rightMatrix->m42; result->m43 = leftMatrix->m43 + rightMatrix->m43; result->m44 = leftMatrix->m44 + rightMatrix->m44; 

    return (beacon_oop_t)result;
}

static beacon_oop_t beacon_Matrix4x4_minus(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.matrix4x4Class);

    beacon_Matrix4x4_t *leftMatrix  = (beacon_Matrix4x4_t *)receiver;
    beacon_Matrix4x4_t *rightMatrix = (beacon_Matrix4x4_t *)arguments[0];
    beacon_Matrix4x4_t *result = beacon_allocateObjectWithBehavior(context->heap, context->classes.matrix4x4Class, sizeof(beacon_Matrix4x4_t), BeaconObjectKindBytes);
    result->m11 = leftMatrix->m11 - rightMatrix->m11; result->m12 = leftMatrix->m12 - rightMatrix->m12; result->m13 = leftMatrix->m13 - rightMatrix->m13; result->m14 = leftMatrix->m14 - rightMatrix->m14;
    result->m21 = leftMatrix->m21 - rightMatrix->m21; result->m22 = leftMatrix->m22 - rightMatrix->m22; result->m23 = leftMatrix->m23 - rightMatrix->m23; result->m24 = leftMatrix->m24 - rightMatrix->m24;
    result->m31 = leftMatrix->m31 - rightMatrix->m31; result->m32 = leftMatrix->m32 - rightMatrix->m32; result->m33 = leftMatrix->m33 - rightMatrix->m33; result->m34 = leftMatrix->m34 - rightMatrix->m34; 
    result->m41 = leftMatrix->m41 - rightMatrix->m41; result->m42 = leftMatrix->m42 - rightMatrix->m42; result->m43 = leftMatrix->m43 - rightMatrix->m43; result->m44 = leftMatrix->m44 - rightMatrix->m44; 

    return (beacon_oop_t)result;
}

void beacon_context_registerLinearAlgebraPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.vector2Class), "x:y:", 2, beacon_Vector2_constructWithXY);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "x", 0, beacon_Vector2_x);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "y", 0, beacon_Vector2_y);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "dot:", 1, beacon_Vector2_dot);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "length", 0, beacon_Vector2_length);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "negated", 0, beacon_Vector2_negated);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "+", 1, beacon_Vector2_add);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "-", 1, beacon_Vector2_minus);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "*", 1, beacon_Vector2_times);
    beacon_addPrimitiveToClass(context, context->classes.vector2Class, "/", 1, beacon_Vector2_divide);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.vector3Class), "x:y:z:", 3, beacon_Vector3_constructWithXYZ);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "asVector3", 0, beacon_Vector3_asVector3);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "x", 0, beacon_Vector3_x);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "y", 0, beacon_Vector3_y);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "z", 0, beacon_Vector3_z);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "dot:", 1, beacon_Vector3_dot);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "length", 0, beacon_Vector3_length);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "negated", 0, beacon_Vector3_negated);
    beacon_addPrimitiveToClass(context, context->classes.vector3Class, "reciprocal", 0, beacon_Vector3_reciprocal);
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
    beacon_addPrimitiveToClass(context, context->classes.vector4Class, "negated", 0, beacon_Vector4_negated);
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
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "asMatrix3x3", 0, beacon_Quaternion_asMatrix3x3);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "+", 1, beacon_Quaternion_add);
    beacon_addPrimitiveToClass(context, context->classes.quaternionClass, "-", 1, beacon_Quaternion_minus);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.matrix2x2Class), "identity", 0, beacon_Matrix2x2_identity);
    beacon_addPrimitiveToClass(context, context->classes.matrix2x2Class, "firstColumn",  0, beacon_Matrix2x2_firstColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix2x2Class, "secondColumn", 0, beacon_Matrix2x2_secondColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix2x2Class, "firstRow",     0, beacon_Matrix2x2_firstRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix2x2Class, "secondRow",    0, beacon_Matrix2x2_secondRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix2x2Class, "+", 1, beacon_Matrix2x2_add);
    beacon_addPrimitiveToClass(context, context->classes.matrix2x2Class, "-", 1, beacon_Matrix2x2_minus);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.matrix3x3Class), "identity", 0, beacon_Matrix3x3_identity);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.matrix3x3Class), "scale:", 1, beacon_Matrix3x3_scale);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "asMatrix3x3",  0, beacon_Matrix3x3_asMatrix3x3);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "transpose",    0, beacon_Matrix3x3_transpose);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "firstColumn",  0, beacon_Matrix3x3_firstColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "secondColumn", 0, beacon_Matrix3x3_secondColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "thirdColumn",  0, beacon_Matrix3x3_thirdColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "firstRow",     0, beacon_Matrix3x3_firstRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "secondRow",    0, beacon_Matrix3x3_secondRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "thirdRow",     0, beacon_Matrix3x3_thirdRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "+", 1, beacon_Matrix3x3_add);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "-", 1, beacon_Matrix3x3_minus);
    beacon_addPrimitiveToClass(context, context->classes.matrix3x3Class, "*", 1, beacon_Matrix3x3_multiply);


    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.matrix4x4Class), "identity", 0, beacon_Matrix4x4_identity);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "firstColumn",  0, beacon_Matrix4x4_firstColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "secondColumn", 0, beacon_Matrix4x4_secondColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "thirdColumn",  0, beacon_Matrix4x4_thirdColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "fourthColumn", 0, beacon_Matrix4x4_fourthColumn);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "firstRow",     0, beacon_Matrix4x4_firstRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "secondRow",    0, beacon_Matrix4x4_secondRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "thirdRow",     0, beacon_Matrix4x4_thirdRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "fourthRow",    0, beacon_Matrix4x4_fourthRow);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "+", 1, beacon_Matrix4x4_add);
    beacon_addPrimitiveToClass(context, context->classes.matrix4x4Class, "-", 1, beacon_Matrix4x4_minus);
}
