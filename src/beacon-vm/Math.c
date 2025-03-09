#include "Math.h"
#include <math.h>

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_identity(void)
{
    beacon_RenderMatrix4x4_t matrix = {
        .m11 = 1.0f,
        .m22 = 1.0f,
        .m33 = 1.0f,
        .m44 = 1.0f,
    };
    return matrix;
}

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_translation(beacon_RenderVector3_t translation)
{
    beacon_RenderMatrix4x4_t matrix = {
        .m11 = 1.0f, .m14 = translation.x,
        .m22 = 1.0f, .m24 = translation.y,
        .m33 = 1.0f, .m34 = translation.z,
        .m44 = 1.0f,
    };
    return matrix;
}
beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_reverseDepthFrustum(float left, float right, float bottom, float top, float near, float far, bool flipVertically)
{
    float flipYFactor = flipVertically ? -1.0f : 1.0f;

    beacon_RenderMatrix4x4_t matrix = {
		.m11 = 2*near /(right - left), .m12 = 0, .m13 = (right + left) / (right - left), .m14 = 0,
        .m21 = 0, .m22 = flipYFactor*2*near /(top - bottom), .m23 = flipYFactor*(top + bottom) / (top - bottom), .m24 = 0,
        .m31 = 0, .m32 = 0, .m33 = near / (far - near), .m34 = near*far / (far - near),
        .m41 = 0, .m42 = 0, .m43 = -1, .m44 = 0,
    };
    return matrix;
}

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_reverseDepthPerspective(float fovY, float aspectRatio, float near, float far, bool flipVertically)
{
	float fovyRad = fovY *M_PI / 180.0;
    float top = near*tan(fovyRad);
    float right = top * aspectRatio;
	
    return beacon_RenderMatrix4x4_reverseDepthFrustum(-right, right, -top, top, near, far, flipVertically);
}