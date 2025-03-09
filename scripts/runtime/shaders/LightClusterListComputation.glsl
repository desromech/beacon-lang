#line 1

//Clustered forward rendering list shader is heavily based in the one from http://www.aortiz.me/2018/12/21/CG.html by Ángel Ortiz"
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

#define CameraState cameraStates[PushConstants.cameraStateIndex]

const uint MaxNumberOfLightsPerTile = 100u;
const uint BatchSize = 64u;

shared bool[BatchSize] SharedLightIsVisible;
shared vec4[BatchSize] SharedLightPositionOrDirection;
shared float[BatchSize] SharedLightRadius;

float computeBoundingBoxPointDistance2(vec3 bboxMin, vec3 bboxMax, vec3 point)
{
	vec3 closestPoint = clamp(point, bboxMin, bboxMax);
	vec3 delta = closestPoint - point;
	return dot(delta, delta);
}

bool testBoundingBoxWithSphere(vec3 bboxMin, vec3 bboxMax, vec4 sphereCenter, float sphereRadius)
{ 
	if (sphereCenter.w == 0.0)
		return true;
	else
		return computeBoundingBoxPointDistance2(bboxMin, bboxMax, sphereCenter.xyz) <= sphereRadius*sphereRadius;
}

void main()
{
	uvec3 gridExtent = CameraState.lightGridExtent;
	uint tileCount = gridExtent.x*gridExtent.y*gridExtent.z;
	uint tileIndex = gl_GlobalInvocationID.x;
	vec3 tileMin = tileIndex < tileCount ? LightClusters.clusters[tileIndex].min : vec3(0.0);
	vec3 tileMax = tileIndex < tileCount ? LightClusters.clusters[tileIndex].max : vec3(0.0);
	
	uint visibleLightCount = 0;
	uint visibleLightIndices[MaxNumberOfLightsPerTile];
	
	uint lightCount = PushConstants.lightSourceCount;
	
	for(uint batchStartLightIndex = 0u; batchStartLightIndex < lightCount; batchStartLightIndex += BatchSize)
	{
		// Fetch the lights.
		{ 
			uint lightIndex = min(batchStartLightIndex + gl_LocalInvocationIndex, lightCount);
#define thisLight ViewRenderLightSourceData[lightIndex]
			vec3 intensity = thisLight.intensity;

			SharedLightIsVisible[gl_LocalInvocationIndex] = lightIndex < lightCount
				&& thisLight.influenceRadius > 0
				&& (intensity.r > 0.0 || intensity.g > 0.0 || intensity.b > 0.0);
			SharedLightPositionOrDirection[gl_LocalInvocationIndex] = thisLight.positionOrDirection;
			SharedLightRadius[gl_LocalInvocationIndex] = thisLight.influenceRadius;
#undef thisLight
		}
		
		memoryBarrierShared();
		barrier();
		
		for(uint i = 0u; i < BatchSize; ++i)
		{
			if(visibleLightCount < MaxNumberOfLightsPerTile &&
				SharedLightIsVisible[i] &&
				tileIndex < tileCount &&
				testBoundingBoxWithSphere(tileMin, tileMax, SharedLightPositionOrDirection[i], SharedLightRadius[i]))
			{
				uint lightIndex = batchStartLightIndex + i;
				visibleLightIndices[visibleLightCount++] = lightIndex;
			}
		}
	}
	
	barrier();

	if(tileIndex < tileCount)
	{ 
		uint destinationIndex = visibleLightCount > 0 ? atomicAdd(LightClusterLists.listSize, visibleLightCount) : 0;
		for(uint i = 0u; i < visibleLightCount; ++i)
			TileLightIndices.indices[destinationIndex + i] = visibleLightIndices[i];

		LightClusterLists.lists[tileIndex] = uvec2(destinationIndex, visibleLightCount);
	}
}
