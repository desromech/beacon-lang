#line 2


layout(location = 0) flat in int inMaterialIndex;
layout(location = 1) in vec3 inViewPosition;
layout(location = 2) in vec3 inViewNormal;
layout(location = 3) in vec4 inViewTangent4;

layout(location = 4) in vec3 inWorldPosition;
layout(location = 5) in vec3 inWorldNormal;
layout(location = 6) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outNormalGBuffer;
layout(location = 2) out vec4 outSpecularityGBuffer;


void main()
{
    RenderMaterialAttributes materialAttributes;
    if(inMaterialIndex >= 0)
    {
        materialAttributes = RenderMaterialData[inMaterialIndex];
    }
    else
    {
        materialAttributes.emissiveColor = vec3(0.0);
        materialAttributes.baseColor = vec4(1.0, 1.0, 1.0, 1.0);
        materialAttributes.metallicFactor = 1.0;
        materialAttributes.roughnessFactor = 1.0;
        materialAttributes.occlusionFactor = 1.0;
        materialAttributes.isTranslucent = 0;

        materialAttributes.baseColorTextureIndex = -1;
        materialAttributes.normalTextureIndex = -1;
        materialAttributes.emissiveTextureIndex = -1;
        materialAttributes.roughnessMetallicTextureIndex = -1;
        materialAttributes.occlusionTextureIndex = -1;
    }

    vec3 tangentSpaceN = vec3(0.0, 0.0, 1.0);
    if(materialAttributes.normalTextureIndex >= 0)
        tangentSpaceN = texture(sampler2D(AllTextures[materialAttributes.normalTextureIndex], LinearTextureSampler), inTexcoord).xyz *2.0 - 1.0;

    vec3 surfaceTangent = normalize(inViewTangent4.xyz);
    vec3 surfaceNormal = normalize(inViewNormal);
    vec3 surfaceBitangent = normalize(cross(surfaceNormal, surfaceTangent)*inViewTangent4.w);
    mat3 TBN = mat3(surfaceTangent, surfaceBitangent, surfaceNormal);

	vec3 N = normalize(TBN * tangentSpaceN);
	vec3 V = normalize(-inViewPosition);
	vec3 P = inViewPosition;

    vec4 baseColor = materialAttributes.baseColor;
    if(materialAttributes.baseColorTextureIndex >= 0)
        baseColor *= texture(sampler2D(AllTextures[materialAttributes.baseColorTextureIndex], LinearTextureSampler), inTexcoord);

    float occlusionFactor = materialAttributes.occlusionFactor;
    float roughnessFactor = materialAttributes.roughnessFactor;
    float metallicFactor = materialAttributes.metallicFactor;
    if(materialAttributes.roughnessMetallicTextureIndex >= 0)
    {
        vec4 mrSample = texture(sampler2D(AllTextures[materialAttributes.roughnessMetallicTextureIndex], LinearTextureSampler), inTexcoord);
        roughnessFactor *= mrSample.g;
        metallicFactor *= mrSample.b;
    }
    if(materialAttributes.occlusionTextureIndex >= 0)
    {
        occlusionFactor *= texture(sampler2D(AllTextures[materialAttributes.occlusionTextureIndex], LinearTextureSampler), inTexcoord).r;
    }

    SurfaceLightingParameters lightingParams;
    lightingParams.baseColor = baseColor;
    lightingParams.emissiveFactor = materialAttributes.emissiveColor;
    lightingParams.occlusionFactor = occlusionFactor;
    lightingParams.metallicFactor = metallicFactor;
    lightingParams.roughnessFactor = roughnessFactor;
    lightingParams.N = N;
    lightingParams.P = P;
    lightingParams.V = V;
	lightingParams.worldP = inWorldPosition;
    lightingParams.worldSurfaceN = normalize(inWorldNormal);

	if(!gl_FrontFacing)
	{
		lightingParams.N = -lightingParams.N;
		lightingParams.worldSurfaceN = -lightingParams.worldSurfaceN;
	}
    
    vec4 lightedColor = performLightingModelComputation(lightingParams, outNormalGBuffer, outSpecularityGBuffer);
    outColor = lightedColor;
    //outColor = vec4(N*0.5 + 0.5, 1.0);
}
