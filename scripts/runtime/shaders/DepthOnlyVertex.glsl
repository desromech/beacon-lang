#line 2

#define CameraState cameraStates[PushConstants.cameraStateIndex]

void main()
{
    RenderMeshChunk renderChunk = RenderMeshChunkData[gl_InstanceIndex];
    RenderObjectAttributes renderObject = RenderObjectData[renderChunk.renderObjectIndex];
    RenderMeshPrimitiveAttributes meshPrimitive = RenderSubmeshData[renderChunk.renderMeshPrimitiveIndex];

    int vertexIndex = gl_VertexIndex;
    vec3 vertexPosition = unpackVec3(PositionsData[meshPrimitive.firstPositionIndex + vertexIndex]);
    vec4 worldPosition = renderObject.modelMatrix * vec4(vertexPosition, 1.0);
    vec4 viewPosition = CameraState.viewMatrix * worldPosition;

    gl_Position = CameraState.projectionMatrix * viewPosition;
}