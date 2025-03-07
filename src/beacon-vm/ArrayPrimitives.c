#include "ObjectModel.h"
#include "Context.h"
#include "Exceptions.h"

static beacon_oop_t beacon_uint16Array_size(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    beacon_UInt16Array_t *uint16Array = (beacon_UInt16Array_t*)receiver;
    intptr_t arraySize = uint16Array->super.super.super.super.super.header.slotCount / 2;
    return beacon_encodeSmallInteger(arraySize);
}

static beacon_oop_t beacon_uint16Array_at(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    beacon_UInt16Array_t *uint16Array = (beacon_UInt16Array_t*)receiver;
    intptr_t uint16ArraySize = uint16Array->super.super.super.super.super.header.slotCount / 2;
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);

    BeaconAssert(context, 1 <= index && index <= uint16ArraySize);
    return beacon_encodeDoubleAsNumber(context, uint16Array->elements[index - 1]);
}

static beacon_oop_t beacon_uint16Array_atPut(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    BeaconAssert(context, beacon_isImmediate(arguments[1]));
    beacon_UInt16Array_t *uint16Array = (beacon_UInt16Array_t*)receiver;
    intptr_t uint16ArraySize = uint16Array->super.super.super.super.super.header.slotCount / 2;
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);
    intptr_t value = beacon_decodeSmallInteger(arguments[1]);

    BeaconAssert(context, 1 <= index && index <= uint16ArraySize);
    uint16Array->elements[index - 1] = value;
    return receiver;
}

static beacon_oop_t beacon_uint32Array_size(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    beacon_UInt32Array_t *uint32Array = (beacon_UInt32Array_t*)receiver;
    intptr_t arraySize = uint32Array->super.super.super.super.super.header.slotCount / 4;
    return beacon_encodeSmallInteger(arraySize);
}

static beacon_oop_t beacon_uint32Array_at(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    beacon_UInt32Array_t *uint32Array = (beacon_UInt32Array_t*)receiver;
    intptr_t uint32ArraySize = uint32Array->super.super.super.super.super.header.slotCount / 4;
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);

    BeaconAssert(context, 1 <= index && index <= uint32ArraySize);
    return beacon_encodeDoubleAsNumber(context, uint32Array->elements[index - 1]);
}

static beacon_oop_t beacon_uint32Array_atPut(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    BeaconAssert(context, beacon_isImmediate(arguments[1]));
    beacon_UInt32Array_t *uint32Array = (beacon_UInt32Array_t*)receiver;
    intptr_t uint32ArraySize = uint32Array->super.super.super.super.super.header.slotCount / 4;
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);
    intptr_t value = beacon_decodeSmallInteger(arguments[1]);

    BeaconAssert(context, 1 <= index && index <= uint32ArraySize);
    uint32Array->elements[index - 1] = value;
    return receiver;
}

static beacon_oop_t beacon_float32Array_size(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    beacon_Float32Array_t *floatArray = (beacon_Float32Array_t*)receiver;
    intptr_t floatArraySize = floatArray->super.super.super.super.super.header.slotCount / 4;
    return beacon_encodeSmallInteger(floatArraySize);
}

static beacon_oop_t beacon_float32Array_at(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    beacon_Float32Array_t *floatArray = (beacon_Float32Array_t*)receiver;
    intptr_t floatArraySize = floatArray->super.super.super.super.super.header.slotCount / 4;
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);

    BeaconAssert(context, 1 <= index && index <= floatArraySize);
    return beacon_encodeDoubleAsNumber(context, floatArray->elements[index - 1]);
}

static beacon_oop_t beacon_float32Array_atPut(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    beacon_Float32Array_t *floatArray = (beacon_Float32Array_t*)receiver;
    intptr_t floatArraySize = floatArray->super.super.super.super.super.header.slotCount / 4;
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);
    double value = beacon_decodeNumberAsDouble(context, arguments[1]);

    BeaconAssert(context, 1 <= index && index <= floatArraySize);
    floatArray->elements[index - 1] = value;
    return receiver;
}

static beacon_oop_t beacon_float64Array_size(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 0);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    beacon_Float64Array_t *floatArray = (beacon_Float64Array_t*)receiver;
    intptr_t floatArraySize = floatArray->super.super.super.super.super.header.slotCount / 8;
    return beacon_encodeSmallInteger(floatArraySize);
}

static beacon_oop_t beacon_float64Array_at(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    beacon_Float64Array_t *floatArray = (beacon_Float64Array_t*)receiver;
    intptr_t floatArraySize = floatArray->super.super.super.super.super.header.slotCount / 8;
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);

    BeaconAssert(context, 1 <= index && index <= floatArraySize);
    return beacon_encodeDoubleAsNumber(context, floatArray->elements[index - 1]);
}

static beacon_oop_t beacon_float64Array_atPut(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    BeaconAssert(context, !beacon_isImmediate(receiver));
    BeaconAssert(context, beacon_isImmediate(arguments[0]));
    beacon_Float64Array_t *floatArray = (beacon_Float64Array_t*)receiver;
    intptr_t floatArraySize = floatArray->super.super.super.super.super.header.slotCount / 8;
    intptr_t index = beacon_decodeSmallInteger(arguments[0]);
    double value = beacon_decodeNumberAsDouble(context, arguments[1]);

    BeaconAssert(context, 1 <= index && index <= floatArraySize);
    floatArray->elements[index - 1] = value;
    return receiver;
}

void beacon_context_registerArrayPrimitive(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.uint16ArrayClass, "size", 0,    beacon_uint16Array_size);
    beacon_addPrimitiveToClass(context, context->classes.uint16ArrayClass, "at:", 1,     beacon_uint16Array_at);
    beacon_addPrimitiveToClass(context, context->classes.uint16ArrayClass, "at:put:", 2, beacon_uint16Array_atPut);

    beacon_addPrimitiveToClass(context, context->classes.uint32ArrayClass, "size", 0,    beacon_uint32Array_size);
    beacon_addPrimitiveToClass(context, context->classes.uint32ArrayClass, "at:", 1,     beacon_uint32Array_at);
    beacon_addPrimitiveToClass(context, context->classes.uint32ArrayClass, "at:put:", 2, beacon_uint32Array_atPut);

    beacon_addPrimitiveToClass(context, context->classes.float32ArrayClass, "size", 0,    beacon_float32Array_size);
    beacon_addPrimitiveToClass(context, context->classes.float32ArrayClass, "at:", 1,     beacon_float32Array_at);
    beacon_addPrimitiveToClass(context, context->classes.float32ArrayClass, "at:put:", 2, beacon_float32Array_atPut);

    beacon_addPrimitiveToClass(context, context->classes.float64ArrayClass, "size", 0,    beacon_float64Array_size);
    beacon_addPrimitiveToClass(context, context->classes.float64ArrayClass, "at:", 1,     beacon_float64Array_at);
    beacon_addPrimitiveToClass(context, context->classes.float64ArrayClass, "at:put:", 2, beacon_float64Array_atPut);

}
