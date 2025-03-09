#line 2
layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

#define CameraState cameraStates[PushConstants.cameraStateIndex]

vec3 ndcToView(vec4 ndc)
{
	vec4 viewPosition4 = CameraState.inverseProjectionMatrix * ndc;
	return viewPosition4.xyz / viewPosition4.w;
}

float depthForSliceIndex(uint sliceIndex)
{ 
	return -CameraState.nearDistance*pow(CameraState.farDistance/CameraState.nearDistance, float(sliceIndex) / float(CameraState.lightGridExtent.z));
}

vec3 projectNearFrustumCornerToDepth(vec3 corner, float depth)
{
	return corner*depth/-CameraState.nearDistance;
}

void main()
{
	uvec3 gridExtent = CameraState.lightGridExtent;

	// Bounds check
	if(any(greaterThanEqual(gl_GlobalInvocationID, gridExtent)))
		return;
		
	float nearDistanceDepth = 1.0; // Reverse depth.
		
	vec4 ndcBottomLeftCorner = vec4(vec2(gl_GlobalInvocationID.xy) / vec2(gridExtent.xy)*2.0 - 1.0, nearDistanceDepth, 1.0);
	vec4 ndcBottomRightCorner = vec4(vec2(gl_GlobalInvocationID.xy + uvec2(1u, 0u))/ vec2(gridExtent.xy)*2.0 - 1.0, nearDistanceDepth, 1.0);
	vec4 ndcTopLeftCorner = vec4(vec2(gl_GlobalInvocationID.xy + uvec2(0u, 1u))/ vec2(gridExtent.xy)*2.0 - 1.0, nearDistanceDepth, 1.0);
	vec4 ndcTopRightCorner = vec4(vec2(gl_GlobalInvocationID.xy + 1u) / vec2(gridExtent.xy)*2.0 - 1.0, nearDistanceDepth, 1.0);
	
	if(!CameraState.hasTopLeftNDCOrigin)
	{ 
		ndcBottomLeftCorner.y = -ndcBottomLeftCorner.y;
		ndcBottomRightCorner.y = -ndcBottomRightCorner.y;
		ndcTopLeftCorner.y = -ndcTopLeftCorner.y;
		ndcTopRightCorner.y = -ndcTopRightCorner.y;
	}
	
	vec3 viewBottomLeftCorner = ndcToView(ndcBottomLeftCorner);
	vec3 viewBottomRightCorner = ndcToView(ndcBottomRightCorner);
	vec3 viewTopLeftCorner = ndcToView(ndcTopLeftCorner);
	vec3 viewTopRightCorner = ndcToView(ndcTopRightCorner);
	
	float tileNearDistance = depthForSliceIndex(gl_GlobalInvocationID.z);
	float tileFarDistance = depthForSliceIndex(gl_GlobalInvocationID.z + 1u);
	
	vec3 tileNearBottomLeftCorner = projectNearFrustumCornerToDepth(viewBottomLeftCorner, tileNearDistance);
	vec3 tileNearBottomRightCorner = projectNearFrustumCornerToDepth(viewBottomRightCorner, tileNearDistance);
	vec3 tileNearTopLeftCorner = projectNearFrustumCornerToDepth(viewTopLeftCorner, tileNearDistance);
	vec3 tileNearTopRightCorner = projectNearFrustumCornerToDepth(viewTopRightCorner, tileNearDistance);

	vec3 tileFarBottomLeftCorner = projectNearFrustumCornerToDepth(viewBottomLeftCorner, tileFarDistance);
	vec3 tileFarBottomRightCorner = projectNearFrustumCornerToDepth(viewBottomRightCorner, tileFarDistance);
	vec3 tileFarTopLeftCorner = projectNearFrustumCornerToDepth(viewTopLeftCorner, tileFarDistance);
	vec3 tileFarTopRightCorner = projectNearFrustumCornerToDepth(viewTopRightCorner, tileFarDistance);

	LightCluster cluster;
	cluster.min = min(
			min(min(tileNearBottomLeftCorner, tileNearBottomRightCorner),
			min(tileNearTopLeftCorner, tileNearTopRightCorner)),

			min(min(tileFarBottomLeftCorner, tileFarBottomRightCorner),
			min(tileFarTopLeftCorner, tileFarTopRightCorner))
	);
	cluster.max = max(
			max(max(tileNearBottomLeftCorner, tileNearBottomRightCorner),
			max(tileNearTopLeftCorner, tileNearTopRightCorner)),

			max(max(tileFarBottomLeftCorner, tileFarBottomRightCorner),
			max(tileFarTopLeftCorner, tileFarTopRightCorner))
	);

	uint tileIndex = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y*gridExtent.x + gl_GlobalInvocationID.z*gridExtent.x*gridExtent.y;
	LightClusters.clusters[tileIndex] = cluster;
}
