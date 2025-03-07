#ifndef BEACON_MATH_H
#define BEACON_MATH_H

typedef struct beacon_RenderVector2_s
{
    float x, y;
} beacon_RenderVector2_t;

typedef struct beacon_RenderVector3_s
{
    float x, y, z;
} beacon_RenderVector3_t;

typedef struct beacon_RenderVector4_s
{
    float x, y, z, w;
} beacon_RenderVector4_t;

typedef struct beacon_RenderMatrix4x4_s
{
    float m11; float m21; float m31; float m41;
    float m12; float m22; float m32; float m42;
    float m13; float m23; float m33; float m43;
    float m14; float m24; float m34; float m44;
} beacon_RenderMatrix4x4_t;

#endif //BEACON_MATH_H
