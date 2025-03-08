#ifndef BEACON_MATH_H
#define BEACON_MATH_H

typedef struct beacon_RenderVector2_s
{
    float x, y;
} beacon_RenderVector2_t;

typedef struct beacon_RenderVector3_s
{
    float x, y, z;
    float padding;
} beacon_RenderVector3_t;

typedef struct beacon_RenderPackedVector3_s
{
    float x, y, z;
} beacon_RenderPackedVector3_t;

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

typedef struct beacon_AABox3_s
{
	beacon_RenderVector3_t min;
	beacon_RenderVector3_t max;
} beacon_AABox3_t;

typedef struct beacon_Frustum_s
{
	beacon_RenderVector3_t leftBottomNear;
	beacon_RenderVector3_t rightBottomNear;
	beacon_RenderVector3_t leftTopNear;
	beacon_RenderVector3_t rightTopNear;
	beacon_RenderVector3_t leftBottomFar;
	beacon_RenderVector3_t rightBottomFar;
	beacon_RenderVector3_t leftTopFar;
	beacon_RenderVector3_t rightTopFar;

	beacon_AABox3_t boundingBox;
    beacon_RenderVector4_t planes[6];
} beacon_Frustum_t;
#endif //BEACON_MATH_H
