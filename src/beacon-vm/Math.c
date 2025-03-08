#include "Math.h"

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
