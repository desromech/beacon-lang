#line 2

void main()
{
    RenderMeshChunk renderChunk = RenderMeshChunkData[gl_InstanceIndex];
    RenderObjectAttributes renderObject = RenderObjectData[renderChunk.renderObjectIndex];
    RenderMeshPrimitiveAttributes meshPrimitive = RenderSubmeshData[renderChunk.renderMeshPrimitiveIndex];

    int vertexIndex = gl_VertexIndex;
    vec3 vertexPosition = PositionsData[meshPrimitive.firstPositionIndex + vertexIndex];
    vec4 worldPosition = renderObject.modelMatrix * vec4(vertexPosition, 1.0);
    vec4 viewPosition = cameraStates[PushConstants.cameraStateIndex].viewMatrix * worldPosition;

    gl_Position = cameraStates[PushConstants.cameraStateIndex].projectionMatrix * viewPosition;
}
