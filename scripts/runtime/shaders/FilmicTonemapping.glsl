#line 2

layout(set=3, binding=0) uniform texture2D SourceTexture;

layout(location = 0) in vec2 inTexcoord;
layout(location = 0) out vec4 outColor;

vec3 filmicCurve(vec3 x)
{
	// Filmic tone mapping curve from: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
	vec4 hdrTexel = textureLod(sampler2D(SourceTexture, LinearTextureSampler), inTexcoord, 0.0);
	vec3 hdrColor = hdrTexel.rgb;// * CameraState.exposure;
	vec3 ldrColor = filmicCurve(hdrColor);
	outColor = vec4(ldrColor, hdrTexel.a);
}
