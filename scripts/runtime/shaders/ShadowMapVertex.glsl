#line 2

void main()
{
    RenderMeshChunk renderChunk = RenderMeshChunkData[gl_InstanceIndex];
    RenderObjectAttributes renderObject = RenderObjectData[renderChunk.renderObjectIndex];
    RenderMeshPrimitiveAttributes meshPrimitive = RenderSubmeshData[renderChunk.renderMeshPrimitiveIndex];

    int vertexIndex = gl_VertexIndex;
    vec3 vertexPosition = unpackVec3(PositionsData[meshPrimitive.firstPositionIndex + vertexIndex]);
    vec4 worldPosition = renderObject.modelMatrix * vec4(vertexPosition, 1.0);
    
    vec4 viewPosition = WorldRenderLightSourceData[PushConstants.shadowMapLightSourceIndex].inverseModelMatrix[PushConstants.shadowMapComponent] * worldPosition;
    gl_Position = WorldRenderLightSourceData[PushConstants.shadowMapLightSourceIndex].projectionMatrix[PushConstants.shadowMapComponent] * viewPosition;
}