#line 2

layout(location = 0) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outNormalGBuffer;
layout(location = 2) out vec4 outSpecularityGBuffer;

void main()
{
    outColor = vec4(mix(vec3(0.01, 0.01, 0.01), vec3(0.2, 0.2, 0.8), 1.0 - inTexcoord.y), 1.0);
    outNormalGBuffer = vec2(0.0, 0.0);
    outSpecularityGBuffer = vec4(0.0, 0.0, 0.0, 0.0);
}
