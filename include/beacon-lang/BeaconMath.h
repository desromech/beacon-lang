#ifndef BEACON_MATH_H
#define BEACON_MATH_H

#include <stdbool.h>
#include "ObjectModel.h"
#include <math.h>

#ifdef _MSC_VER
#define GCC_ALIGNED(x)
#else
#define GCC_ALIGNED(x) __attribute__((aligned(x)))
#endif

#ifdef _MSC_VER
#pragma pack(push, 8)
#endif

typedef struct beacon_RenderVector2_s
{
    float x, y;
} beacon_RenderVector2_t GCC_ALIGNED(8);

#ifdef _MSC_VER
#pragma pack(pop)
#pragma pack(push, 16)
#endif

typedef struct beacon_RenderVector3_s
{
    float x, y, z, padding;
} beacon_RenderVector3_t GCC_ALIGNED(16);

#ifdef _MSC_VER
#pragma pack(pop)
#endif

static inline beacon_RenderVector3_t beacon_RenderVector3_make(float x, float y, float z)
{
	beacon_RenderVector3_t result = {x, y, z, 0};
	return result;
}

typedef struct beacon_RenderPackedVector3_s
{
    float x, y, z;
} beacon_RenderPackedVector3_t;

#ifdef _MSC_VER
#pragma pack(push, 16)
#endif

typedef union beacon_RenderVector4_s
{
    struct
	{
		float x, y, z, w;
	};

} beacon_RenderVector4_t GCC_ALIGNED(16);

#ifdef _MSC_VER
#pragma pack(pop)
#pragma pack(push, 16)
#endif

typedef union beacon_RenderQuaternion_s
{
    struct
	{
		float x, y, z, w;
	};

} beacon_RenderQuaternion_t GCC_ALIGNED(16);

#ifdef _MSC_VER
#pragma pack(pop)
#endif

static inline beacon_RenderQuaternion_t beacon_RenderQuaternion_conjugated(const beacon_RenderQuaternion_t quat)
{
	beacon_RenderQuaternion_t result = {-quat.x, -quat.y, -quat.z, quat.w};
	return result;
}

#ifdef _MSC_VER
#pragma pack(push, 16)
#endif

typedef union beacon_RenderMatrix3x3_s
{
	struct {
		float m11; float m21; float m31; float _pading;
		float m12; float m22; float m32; float _pading1;
		float m13; float m23; float m33; float _pading2;
		float m14; float m24; float m34; float _pading3;		
	};

	beacon_RenderVector3_t columns[3];
} beacon_RenderMatrix3x3_t GCC_ALIGNED(16);

#ifdef _MSC_VER
#pragma pack(pop)
#endif

static inline beacon_RenderMatrix3x3_t beacon_RenderMatrix3x3_makeWithColumns(beacon_RenderVector3_t first, beacon_RenderVector3_t second, beacon_RenderVector3_t third)
{
	beacon_RenderMatrix3x3_t result = {
		.columns = {first, second, third}
	};
	return result;
}

#ifdef _MSC_VER
#pragma pack(push, 16)
#endif

typedef union beacon_RenderMatrix4x4_s
{
	struct {
		float m11; float m21; float m31; float m41;
		float m12; float m22; float m32; float m42;
		float m13; float m23; float m33; float m43;
		float m14; float m24; float m34; float m44;		
	};

	beacon_RenderVector4_t columns[4];
} beacon_RenderMatrix4x4_t GCC_ALIGNED(16);

#ifdef _MSC_VER
#pragma pack(pop)
#endif

typedef struct beacon_AABox3_s
{
	beacon_RenderVector3_t min;
	beacon_RenderVector3_t max;
} beacon_AABox3_t;

static inline beacon_RenderVector3_t beacon_RenderVector3_min(beacon_RenderVector3_t a, beacon_RenderVector3_t b)
{
	beacon_RenderVector3_t result = {
		.x = a.x < b.x ? a.x : b.x,
		.y = a.y < b.y ? a.y : b.y,
		.z = a.z < b.z ? a.z : b.z,
	};
	return result;
}

static inline beacon_RenderVector3_t beacon_RenderVector3_max(beacon_RenderVector3_t a, beacon_RenderVector3_t b)
{
	beacon_RenderVector3_t result = {
		.x = a.x > b.x ? a.x : b.x,
		.y = a.y > b.y ? a.y : b.y,
		.z = a.z > b.z ? a.z : b.z,
	};
	return result;
}

static inline beacon_AABox3_t beacon_AABox3_empty(void)
{
	beacon_AABox3_t result = {
		.min = {INFINITY, INFINITY, INFINITY},
		.max = {-INFINITY, -INFINITY, -INFINITY},
	};
	return result;
}

static inline void beacon_AABox3_insertPoint(beacon_AABox3_t *box, beacon_RenderVector3_t point)
{
	box->min = beacon_RenderVector3_min(box->min, point);
	box->max = beacon_RenderVector3_max(box->max, point);
}

static inline beacon_RenderVector3_t beacon_AABox3_center(const beacon_AABox3_t *box)
{
	beacon_RenderVector3_t result = {
		.x = box->min.x + (box->max.x - box->min.x) * 0.5f,
		.y = box->min.y + (box->max.y - box->min.y) * 0.5f,
		.z = box->min.z + (box->max.z - box->min.z) * 0.5f,
	};
	return result;
}

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

beacon_RenderMatrix3x3_t beacon_RenderMatrix3x3_fromQuaternion(beacon_RenderQuaternion_t quaternion);
beacon_RenderVector3_t beacon_RenderMatrix3x3_multiplyVector(beacon_RenderMatrix3x3_t mat, beacon_RenderVector3_t v);

float beacon_RenderMatrix3x3_determinant(beacon_RenderMatrix3x3_t mat);
extern beacon_RenderMatrix3x3_t beacon_RenderMatrix3x3_CubeMapFaceRotations[6];

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_withMatrix3x3AndTranslation(beacon_RenderMatrix3x3_t mat, beacon_RenderVector3_t translation);

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_fromMatrix4x4(beacon_Matrix4x4_t *mat);
beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_translation(beacon_RenderVector3_t translation);
beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_identity(void);

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_multiply(beacon_RenderMatrix4x4_t left, beacon_RenderMatrix4x4_t right);
beacon_RenderVector3_t beacon_RenderMatrix4x4_multiplyVector3(beacon_RenderMatrix4x4_t mat, beacon_RenderVector3_t vec);

float beacon_RenderMatrix4x4_determinant(beacon_RenderMatrix4x4_t mat);
beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_inverse(beacon_RenderMatrix4x4_t mat);

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_reverseDepthOrtho(float left, float right, float bottom, float top, float near, float far);
beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_reverseDepthFrustum(float left, float right, float bottom, float top, float near, float far, bool flipVertically);
beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_reverseDepthPerspective(float fovY, float aspectRatio, float near, float far, bool flipVertically);

void beacon_Frustum_setFrustumTangents(beacon_Frustum_t *frustum, float left, float right, float bottom, float top, float nearDistance, float farDistance);
void beacon_Frustum_setPerspective(beacon_Frustum_t *frustum, float fovy, float aspect, float nearDistance, float farDistance);
void beacon_Frustum_transformWithMatrix4x4(beacon_Frustum_t *outTransformedFrustum, const beacon_Frustum_t *inFrustum, beacon_RenderMatrix4x4_t matrix);

void beacon_Frustum_computeBoundingBox(beacon_Frustum_t *frustum);
beacon_Frustum_t beacon_Frustum_splitAtNearAndFarLambda(const beacon_Frustum_t *inFrustum, float nearLambda, float farLambda);

#endif //BEACON_MATH_H
