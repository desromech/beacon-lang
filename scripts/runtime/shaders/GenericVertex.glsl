#line 2

#define CameraState cameraStates[PushConstants.cameraStateIndex]

layout(location = 0) flat out int outMaterialIndex;
layout(location = 1) out vec3 outViewPosition;
layout(location = 2) out vec3 outViewNormal;
layout(location = 3) out vec4 outViewTangent4;
layout(location = 4) flat out uint hasTangent4;

layout(location = 5) out vec3 outWorldPosition;
layout(location = 6) out vec3 outWorldNormal;
layout(location = 7) out vec2 outTexcoord;

void main()
{
    RenderMeshChunk renderChunk = RenderMeshChunkData[gl_InstanceIndex];
    RenderObjectAttributes renderObject = RenderObjectData[renderChunk.renderObjectIndex];
    RenderMeshPrimitiveAttributes meshPrimitive = RenderSubmeshData[renderChunk.renderMeshPrimitiveIndex];
    outMaterialIndex = meshPrimitive.materialIndex;

    int vertexIndex = gl_VertexIndex;
    vec3 vertexPosition = unpackVec3(PositionsData[meshPrimitive.firstPositionIndex + vertexIndex]);
    vec3 vertexNormal = meshPrimitive.firstNormalIndex >= 0
        ? unpackVec3(NormalsData[meshPrimitive.firstNormalIndex + vertexIndex])
        : vec3(0.0, 0.0, 1.0);
    vec2 vertexTexcoord = meshPrimitive.firstTexcoordIndex >= 0
        ? TexcoordsData[meshPrimitive.firstTexcoordIndex + vertexIndex]
        : vec2(0.0, 0.0);
    outTexcoord = vertexTexcoord;

    hasTangent4 = meshPrimitive.firstTangents4Index >= 0 ? 1 : 0;
    vec4 vertexTangents4 = meshPrimitive.firstTangents4Index >= 0
        ? Tangents4Data[meshPrimitive.firstTangents4Index + vertexIndex]
        : vec4(1.0, 0.0, 0.0, 1.0);

    vec4 worldPosition = renderObject.modelMatrix * vec4(vertexPosition, 1.0);
    vec4 viewPosition = CameraState.viewMatrix * worldPosition;
    outWorldPosition = worldPosition.xyz;
    outViewPosition = viewPosition.xyz;

    vec4 worldNormal = renderObject.modelMatrix * vec4(vertexNormal, 0.0);
    vec4 viewNormal = CameraState.viewMatrix * worldNormal;
    outWorldNormal = worldNormal.xyz;
    outViewNormal = viewNormal.xyz;

    vec4 worldTangent = renderObject.modelMatrix * vec4(vertexTangents4.xyz, 0.0);
    vec4 viewTangent = CameraState.viewMatrix *worldTangent;
    outViewTangent4 = vec4(viewTangent.xyz, vertexTangents4.w);


    gl_Position = CameraState.projectionMatrix * viewPosition;
}