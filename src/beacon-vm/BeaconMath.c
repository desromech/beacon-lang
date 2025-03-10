#include "BeaconMath.h"
#include <math.h>
#include <stdlib.h>

float beacon_RenderMatrix3x3_determinant(beacon_RenderMatrix3x3_t m)
{
    //| m11 m12 m13 | m11 m12
	//| m21 m22 m23 | m21 m22
	//| m31 m32 m33 | m31 m32

    return 
        (m.m11 * m.m22 * m.m33) + (m.m12 * m.m23 * m.m31) + (m.m13 * m.m21 *m.m32)
        - (m.m31 * m.m22 * m.m13) - (m.m32 * m.m23 * m.m11) - (m.m33 * m.m21 * m.m12);
}

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

beacon_RenderVector3_t beacon_renderVector4_minorAt(beacon_RenderVector4_t vector, int index)
{
    switch(index)
    {
    case 0: return beacon_RenderVector3_make(vector.y, vector.z, vector.w);
    case 1: return beacon_RenderVector3_make(vector.x, vector.z, vector.w);
    case 2: return beacon_RenderVector3_make(vector.x, vector.y, vector.w);
    case 3: return beacon_RenderVector3_make(vector.x, vector.y, vector.z);
    default: abort();
    }
}

beacon_RenderMatrix3x3_t beacon_renderMatrix4x4_minorMatrixAt(beacon_RenderMatrix4x4_t mat4x4, int row, int column)
{

    switch(column)
    {
    case 0: return beacon_RenderMatrix3x3_makeWithColumns(
            beacon_renderVector4_minorAt(mat4x4.columns[1], row),
            beacon_renderVector4_minorAt(mat4x4.columns[2], row),
            beacon_renderVector4_minorAt(mat4x4.columns[3], row)
        );
    case 1: return beacon_RenderMatrix3x3_makeWithColumns(
            beacon_renderVector4_minorAt(mat4x4.columns[0], row),
            beacon_renderVector4_minorAt(mat4x4.columns[2], row),
            beacon_renderVector4_minorAt(mat4x4.columns[3], row)
        );
    case 2: return beacon_RenderMatrix3x3_makeWithColumns(
            beacon_renderVector4_minorAt(mat4x4.columns[0], row),
            beacon_renderVector4_minorAt(mat4x4.columns[1], row),
            beacon_renderVector4_minorAt(mat4x4.columns[3], row)
        );
    case 3: return beacon_RenderMatrix3x3_makeWithColumns(
            beacon_renderVector4_minorAt(mat4x4.columns[0], row),
            beacon_renderVector4_minorAt(mat4x4.columns[1], row),
            beacon_renderVector4_minorAt(mat4x4.columns[2], row)
       );
    default: abort();
    }
}

float beacon_renderMatrix4x4_minorAt(beacon_RenderMatrix4x4_t mat4x4, int row, int column)
{
    return beacon_RenderMatrix3x3_determinant(beacon_renderMatrix4x4_minorMatrixAt(mat4x4, row, column));
}

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_multiply(beacon_RenderMatrix4x4_t left, beacon_RenderMatrix4x4_t right)
{
    beacon_RenderMatrix4x4_t result = {
        .m11 = left.m11*right.m11 + left.m12*right.m21 + left.m13*right.m31 + left.m14*right.m41,
        .m12 = left.m11*right.m12 + left.m12*right.m22 + left.m13*right.m32 + left.m14*right.m42,
        .m13 = left.m11*right.m13 + left.m12*right.m23 + left.m13*right.m33 + left.m14*right.m43,
        .m14 = left.m11*right.m14 + left.m12*right.m24 + left.m13*right.m34 + left.m14*right.m44,

        .m21 = left.m21*right.m11 + left.m22*right.m21 + left.m23*right.m31 + left.m24*right.m41,
        .m22 = left.m21*right.m12 + left.m22*right.m22 + left.m23*right.m32 + left.m24*right.m42,
        .m23 = left.m21*right.m13 + left.m22*right.m23 + left.m23*right.m33 + left.m24*right.m43,
        .m24 = left.m21*right.m14 + left.m22*right.m24 + left.m23*right.m34 + left.m24*right.m44,

        .m31 = left.m31*right.m11 + left.m32*right.m21 + left.m33*right.m31 + left.m34*right.m41,
        .m32 = left.m31*right.m12 + left.m32*right.m22 + left.m33*right.m32 + left.m34*right.m42,
        .m33 = left.m31*right.m13 + left.m32*right.m23 + left.m33*right.m33 + left.m34*right.m43,
        .m34 = left.m31*right.m14 + left.m32*right.m24 + left.m33*right.m34 + left.m34*right.m44,

        .m41 = left.m41*right.m11 + left.m42*right.m21 + left.m43*right.m31 + left.m44*right.m41,
        .m42 = left.m41*right.m12 + left.m42*right.m22 + left.m43*right.m32 + left.m44*right.m42,
        .m43 = left.m41*right.m13 + left.m42*right.m23 + left.m43*right.m33 + left.m44*right.m43,
        .m44 = left.m41*right.m14 + left.m42*right.m24 + left.m43*right.m34 + left.m44*right.m44,
    };
    return result;
}

float beacon_RenderMatrix4x4_determinant(beacon_RenderMatrix4x4_t mat)
{
    return
      beacon_renderMatrix4x4_minorAt(mat, 0, 0)*mat.columns[0].x
    - beacon_renderMatrix4x4_minorAt(mat, 0, 1)*mat.columns[1].x
    + beacon_renderMatrix4x4_minorAt(mat, 0, 2)*mat.columns[2].x
    - beacon_renderMatrix4x4_minorAt(mat, 0, 3)*mat.columns[3].x;
}

float beacon_RenderMatrix4x4_cofactorAt(beacon_RenderMatrix4x4_t mat, int row, int column)
{
    return beacon_renderMatrix4x4_minorAt(mat, column, row) * ((row + column) & 1 ? -1 : 1);
}

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_adjugate(beacon_RenderMatrix4x4_t mat)
{
    beacon_RenderMatrix4x4_t adjugate = {
        .m11 = beacon_RenderMatrix4x4_cofactorAt(mat, 0, 0),
        .m12 = beacon_RenderMatrix4x4_cofactorAt(mat, 0, 1),
        .m13 = beacon_RenderMatrix4x4_cofactorAt(mat, 0, 2),
        .m14 = beacon_RenderMatrix4x4_cofactorAt(mat, 0, 3),

        .m21 = beacon_RenderMatrix4x4_cofactorAt(mat, 1, 0),
        .m22 = beacon_RenderMatrix4x4_cofactorAt(mat, 1, 1),
        .m23 = beacon_RenderMatrix4x4_cofactorAt(mat, 1, 2),
        .m24 = beacon_RenderMatrix4x4_cofactorAt(mat, 1, 3),

        .m31 = beacon_RenderMatrix4x4_cofactorAt(mat, 2, 0),
        .m32 = beacon_RenderMatrix4x4_cofactorAt(mat, 2, 1),
        .m33 = beacon_RenderMatrix4x4_cofactorAt(mat, 2, 2),
        .m34 = beacon_RenderMatrix4x4_cofactorAt(mat, 2, 3),

        .m41 = beacon_RenderMatrix4x4_cofactorAt(mat, 3, 0),
        .m42 = beacon_RenderMatrix4x4_cofactorAt(mat, 3, 1),
        .m43 = beacon_RenderMatrix4x4_cofactorAt(mat, 3, 2),
        .m44 = beacon_RenderMatrix4x4_cofactorAt(mat, 3, 3),
    };

    return adjugate;
}

beacon_RenderMatrix4x4_t beacon_RenderMatrix4x4_inverse(beacon_RenderMatrix4x4_t mat)
{
    float det = beacon_RenderMatrix4x4_determinant(mat);
    beacon_RenderMatrix4x4_t adj = beacon_RenderMatrix4x4_adjugate(mat);
    beacon_RenderMatrix4x4_t inverse = {
        .m11 = adj.m11 / det, .m12 = adj.m12 / det, .m13 = adj.m13 / det, .m14 = adj.m14 / det,
        .m21 = adj.m21 / det, .m22 = adj.m22 / det, .m23 = adj.m23 / det, .m24 = adj.m24 / det,
        .m31 = adj.m31 / det, .m32 = adj.m32 / det, .m33 = adj.m33 / det, .m34 = adj.m34 / det,
        .m41 = adj.m41 / det, .m42 = adj.m42 / det, .m43 = adj.m43 / det, .m44 = adj.m44 / det,
    };

    return inverse;
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
	float fovyRad = fovY *(M_PI / 180.0) / 2;
    float top = near*tan(fovyRad);
    float right = top * aspectRatio;
	
    return beacon_RenderMatrix4x4_reverseDepthFrustum(-right, right, -top, top, near, far, flipVertically);
}