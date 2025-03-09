#line 4
#define CameraState cameraStates[PushConstants.cameraStateIndex]

const float DistanceEpsilon = 0.00001;

const float Pi = 3.141592653589793;
const float PiReciprocal = 0.3183098861837907;

struct ShadowMappingSamplingData
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
};

const uint PcfMinSampleCount = 5u;
const uint PcfMaxSampleCount = 16u;
const uint PcfMaxAvailableSampleCount = 16u;

const vec2[PcfMaxAvailableSampleCount] PcfSamplingDisk = vec2[PcfMaxAvailableSampleCount](
	vec2(0.0, 0.0),
	vec2(0.5112106443900664, -0.08269973615310144),
	vec2(-0.9059107675710277, 0.3577294337366379),
	vec2(0.35859281167322443, 0.8693857918816552),
	vec2(-0.23316869849021016, -0.8663155249628777),
	vec2(0.0655344748243385, -0.5620816273438193),
	vec2(-0.16502805108438623, 0.37354542472099217),
	vec2(0.8206416609793163, 0.5243960793709364),
	vec2(-0.45458006647163074, -0.12717718869781924),
	vec2(0.9732842240358164, -0.012046630034244887),
	vec2(-0.4458363994238136, 0.8276348839642642),
	vec2(0.6556346005087879, -0.7492692488009433),
	vec2(-0.6804649548979779, -0.5744969703138326),
	vec2(0.3779618262210682, 0.4044132974950658),
	vec2(-0.8987948111718497, -0.04439136527683185),
	vec2(-0.032797453474624705, 0.773199451981671)
);

float sampleShadowMapForLightWithIndexAtPoint(uint lightIndex, vec3 worldSamplePosition)
{
#define viewLightSource ViewRenderLightSourceData[lightIndex]
#define lightSource WorldRenderLightSourceData[lightIndex]
	
	vec4 lightProjectedPosition;
	uint samplingLayer = 0u;
	
	
	// Is this a non-directional light?
	if(lightSource.positionOrDirection.w != 0.0)
	{ 
		if(lightSource.innerSpotCosCutoff > -1.0)
		{ 
			// Spot light.
			lightProjectedPosition = lightSource.projectionMatrix[0] * (lightSource.inverseModelMatrix[0] * vec4(worldSamplePosition, 1.0));
		}
		else
		{
			// Omni-directional. Select the adequate cube map face.
			vec3 lightVector = worldSamplePosition - lightSource.modelMatrix[0][3].xyz;			
			vec3 absLightVector = abs(lightVector);
			if(absLightVector.x >= absLightVector.y)
			{
				if(absLightVector.x >= absLightVector.z)
					samplingLayer = lightVector.x < 0.0 ? 1u : 0u;
				else
					samplingLayer = lightVector.z < 0.0 ? 5u : 4u;
			}
			else
			{ 
				if(absLightVector.y >= absLightVector.z)
					samplingLayer = lightVector.y < 0.0 ? 3u : 2u;
				else
					samplingLayer = lightVector.z < 0.0 ? 5u : 4u;
			}
			
			
			lightProjectedPosition = lightSource.projectionMatrix[samplingLayer] * (lightSource.inverseModelMatrix[samplingLayer] * vec4(worldSamplePosition, 1.0));
		}
	}
	else
	{
		// Select the directional shadow map cascade.
		float cascadeDistance = dot(lightSource.shadowMapCascadeDistanceWorldTransform, vec4(worldSamplePosition, 1.0));
		vec4 cascadeOffsets = lightSource.shadowMapCascadeOffsets;
		if(cascadeDistance <= cascadeOffsets.y)
		{
			if(cascadeDistance <= cascadeOffsets.x)
				samplingLayer = 0u;
			else
				samplingLayer = 1u;
		}
		else
		{ 
			if(cascadeDistance <= cascadeOffsets.z)
				samplingLayer = 2u;
			else
				samplingLayer = 3u;
		}
		
		// Apply the transform to the corresponding cascade.
		lightProjectedPosition = lightSource.projectionMatrix[samplingLayer] * (lightSource.inverseModelMatrix[samplingLayer] * vec4(worldSamplePosition, 1.0));
	}
	
	// Avoid division by zero
	if (lightProjectedPosition.w == 0.0) 
		return 0.0;

	// Clip to unit cube.
	vec3 samplePosition = lightProjectedPosition.xyz / lightProjectedPosition.w;
	if(samplePosition.x < -1.0 || samplePosition.x > 1.0
		|| samplePosition.y < -1.0 || samplePosition.y > 1.0)
		return 0.0;
		
	vec3 atlasSamplePosition = vec3(samplePosition.xy*lightSource.shadowMapViewportScale + lightSource.shadowMapViewportOffsets[samplingLayer], samplePosition.z);

	return textureLod(sampler2DShadow(ShadowMapAtlasTexture, ShadowMapSampler), atlasSamplePosition, 0.0);

#undef lightSource
}

void computeShadowMappingSamplingData(out ShadowMappingSamplingData samplingData, vec3 worldPosition, vec3 worldNormal)
{
	samplingData.position = worldPosition;
	samplingData.normal = worldNormal;
	
	float randomFloat = fract(sin(dot(worldPosition, vec3(12.9898, 78.233, 217.179))) * 43758.5453);
	float s = randomFloat*2.0 - 1.0;
	float c = sqrt(1.0 - s*s);
	vec3 upVector = vec3(c, s, 0.0);
	if(abs(dot(upVector, samplingData.normal)) > 0.9)
		upVector = vec3(-s, c, 0.0);

	samplingData.tangent = normalize(cross(upVector, samplingData.normal));
	samplingData.bitangent = cross(samplingData.normal, samplingData.tangent);
}

float sampleShadowMapForLightWithIndex(in ShadowMappingSamplingData samplingData, uint lightIndex, vec3 lightVector, float lightDistance, float lightMaxDistance)
{
	vec3 sampleCenterPoint = samplingData.position + samplingData.normal*WorldRenderLightSourceData[lightIndex].shadowMapNormalBiasFactor;

	float sampleSum = 0.0;
	uint sampleCount = 0u;
	
	float diskRadius = mix(0.002, 0.02, lightDistance / lightMaxDistance);

	// Perform an initial number of samples for fully in shadow
	for(; sampleCount < PcfMinSampleCount; ++sampleCount)
	{
		vec2 sampleVector = PcfSamplingDisk[sampleCount]*diskRadius;
		vec3 samplePosition = sampleCenterPoint + samplingData.tangent*sampleVector.x + samplingData.bitangent*sampleVector.y;

		sampleSum += sampleShadowMapForLightWithIndexAtPoint(lightIndex, samplePosition);
	}
	
	// Check for fully shadowed or in light zones.
	if(sampleSum == 0.0)
		return 0.0;
	else if(sampleSum == float(PcfMinSampleCount))
		return 1.0;

	// Perform the remaining samples.
	for(; sampleCount < PcfMaxSampleCount; ++sampleCount)
	{
		vec2 sampleVector = PcfSamplingDisk[sampleCount]*diskRadius;
		vec3 samplePosition = sampleCenterPoint + samplingData.tangent*sampleVector.x + samplingData.bitangent*sampleVector.y;

		sampleSum += sampleShadowMapForLightWithIndexAtPoint(lightIndex, samplePosition);
	}

	return sampleSum / float(PcfMaxSampleCount);
}

struct SurfaceLightingParameters
{
	vec4 baseColor;

	vec3 emissiveFactor;
	float occlusionFactor;

	vec3 N;
	float metallicFactor;

	vec3 V;
	float roughnessFactor;

	vec3 P;
	
	vec3 worldP;
	vec3 worldSurfaceN;
};

vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
    float powFactor = 1.0 - cosTheta;
    float powFactor2 = powFactor * powFactor;
    float powFactor4 = powFactor2 * powFactor2;
    float powValue = powFactor4 * powFactor;

    return F0 + (vec3(1.0) - F0) * powValue;
}

vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
	// Function obtained from: https://learnopengl.com/PBR/IBL/Diffuse-irradiance
    float powFactor = 1.0 - cosTheta;
    float powFactor2 = powFactor * powFactor;
    float powFactor4 = powFactor2 * powFactor2;
    float powValue = powFactor4 * powFactor;

    return F0 + (max(vec3(1.0) - roughness, F0) - F0) * powValue;
}

float ggxSpecularDistribution(float alpha, float cosTheta)
{
	float alphaSquare = alpha*alpha;
	float den = cosTheta*cosTheta*(alphaSquare - 1.0) + 1.0;
	return alphaSquare / (Pi * den*den);
}

float smithSchlickBeckmannReciprocalFunction(float k, float cosTheta)
{
    return cosTheta*(1.0 - k) + k;
}

float cookTorranceSmithSchlickGGXMasking(float k, float NdotL, float NdotV)
{
	return 1.0 / (4.0*smithSchlickBeckmannReciprocalFunction(k, NdotL)*smithSchlickBeckmannReciprocalFunction(k, NdotV));
}

float computeLightAttenuation(float distance, float influenceRadius)
{
    float a = distance / max(influenceRadius, DistanceEpsilon);
    float a2 = a*a;
    float a4 = a2*a2;
    float num = clamp(1.0f - a4, 0.0, 1.0);
    return num*num / (distance*distance + 1.0);
}

uint computeLightTileSliceIndexForLinearDepth(float depth)
{
	vec2 scaleAndOffset = CameraState.lightGridDepthSliceScaleOffset;
	return uint(log(-depth)*scaleAndOffset.x + scaleAndOffset.y);
}

uvec3 computeLightTileCoordinateForCurrentScreenCoordinateWithLinearDepth(float depth)
{
	uvec3 gridExtent = CameraState.lightGridExtent;
	vec2 normalizedScreenCoordinate = vec2(gl_FragCoord.xy * CameraState.framebufferReciprocalExtent);
	
	return min(uvec3(uvec2(normalizedScreenCoordinate * vec2(gridExtent.xy)),
		computeLightTileSliceIndexForLinearDepth(depth)), gridExtent - 1u);
}

uint computeLightTileIndexForCurrentScreenCoordinateWithLinearDepth(float depth)
{
	uvec3 gridExtent = CameraState.lightGridExtent;
	uvec3 gridCoordinate = computeLightTileCoordinateForCurrentScreenCoordinateWithLinearDepth(depth);
	
	return gridCoordinate.x + gridCoordinate.y * gridExtent.x + gridCoordinate.z*gridExtent.x*gridExtent.y;
}

vec4 performLightingModelComputation(in SurfaceLightingParameters surfaceParameters, out vec2 gbufferNormal, out vec4 gbufferSpecularity)
{
	vec3 dielectricF0 = vec3(0.04);
	vec3 Cdiffuse = mix(surfaceParameters.baseColor.rgb * (1.0 - dielectricF0), vec3(0.0), surfaceParameters.metallicFactor);
	vec3 diffuse = Cdiffuse * PiReciprocal;
	vec3 F0 = mix(dielectricF0, surfaceParameters.baseColor.rgb, surfaceParameters.metallicFactor);
	
	gbufferNormal = surfaceParameters.N.xy;
	gbufferSpecularity = vec4(F0, surfaceParameters.roughnessFactor);

	float directRoughness = mix(0.01, 1.0, surfaceParameters.roughnessFactor);
	float directAlpha = directRoughness*directRoughness;

	float directKRoughness = (directRoughness + 1.0);
	float directK = directKRoughness*directKRoughness / 8.0;

	float NdotV = max(0.0, dot(surfaceParameters.N, surfaceParameters.V));
			
	vec3 lightedColor = vec3(0);

	mat3 viewToWorld = mat3(CameraState.inverseViewMatrix[0].xyz, CameraState.inverseViewMatrix[1].xyz, CameraState.inverseViewMatrix[2].xyz);
	vec3 worldN = surfaceParameters.N * viewToWorld;
	vec3 worldSurfaceN = surfaceParameters.worldSurfaceN;
	vec3 worldP = surfaceParameters.worldP;

	vec3 R = reflect(-surfaceParameters.V, surfaceParameters.N);
	vec3 worldR = R * viewToWorld;

    // Ambient lighting
    lightedColor += CameraState.ambientLightSource * diffuse;

	// Add the diffuse light probe lighting.
	/*vec3 diffuseLightProbeSample = textureLod(samplerCube(AmbientDiffuseLightProbeTexture, BrdfLutSampler), worldN, 0.0).rgb;
	lightedColor += diffuseLightProbeSample*diffuse;
    */

	lightedColor *= surfaceParameters.occlusionFactor;
	lightedColor += surfaceParameters.emissiveFactor;


	// Direct light accumulation.
	uint tileIndex = computeLightTileIndexForCurrentScreenCoordinateWithLinearDepth(surfaceParameters.P.z);
	uvec2 tileLightList = LightClusterLists.lists[tileIndex];

	// Shadow mapping sampling data
	ShadowMappingSamplingData shadowMappingSamplingData;
	if(tileLightList.y > 0u)
		computeShadowMappingSamplingData(shadowMappingSamplingData, worldP, worldSurfaceN);

	for(uint i = 0u; i < tileLightList.y; ++i)
	{
		uint lightIndex = TileLightIndices.indices[tileLightList.x + i];
	
#define currentLightSource ViewRenderLightSourceData[lightIndex]

		vec4 lightSourcePosition = currentLightSource.positionOrDirection;
		vec3 L = lightSourcePosition.xyz;
		float lightDistance = 0.0;
		float attenuation = 1.0;
		float lightMaxDistance = 1.0;
		if(lightSourcePosition.w != 0)
		{ 
			L = L - surfaceParameters.P;
			lightDistance = length(L);
			L = L/ max(lightDistance, DistanceEpsilon);
			lightMaxDistance = currentLightSource.influenceRadius;
			attenuation = computeLightAttenuation(lightDistance, currentLightSource.influenceRadius);
		}
		else
		{
			L = normalize(L);
		}
		
		if(currentLightSource.innerSpotCosCutoff > -1.0)
		{
			float spotCos = dot(L, currentLightSource.spotDirection);
			attenuation *= smoothstep(currentLightSource.outerSpotCosCutoff, currentLightSource.innerSpotCosCutoff, spotCos);
		}
		
		// Sample the shadow map, if available.
		if (currentLightSource.castShadows)
		{
			float shadowFactor = sampleShadowMapForLightWithIndex(shadowMappingSamplingData, lightIndex, L, lightDistance, lightMaxDistance);
			if(shadowFactor <= 0.0)
				continue;
				
			attenuation *= shadowFactor;
		}

		float NdotL = max(0.0, dot(surfaceParameters.N, L));
		if(NdotL > 0.0 && attenuation > 0.0)
		{
			vec3 H = normalize(L + surfaceParameters.V);
			float NdotH = max(0.0, dot(surfaceParameters.N, H));
			float VdotH = max(0.0, dot(surfaceParameters.V, H));
			
			vec3 F = fresnelSchlick(F0, VdotH);
			float D = ggxSpecularDistribution(directAlpha, NdotH);
			float G = cookTorranceSmithSchlickGGXMasking(directK, NdotL, NdotV);

			lightedColor += (currentLightSource.intensity * (diffuse + F*(D*G))) * (attenuation*NdotL*Pi);
		}
#undef currentLightSource
	}
	
	return vec4(lightedColor, surfaceParameters.occlusionFactor);
}
