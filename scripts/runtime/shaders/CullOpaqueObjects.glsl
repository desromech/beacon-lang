#line 1
layout(local_size_x = 128) in;

void main()
{
    uint invocationIndex = gl_GlobalInvocationID.x;
    if(invocationIndex >= PushConstants.renderObjectsSize)
        return;

    RenderObjectAttributes renderObject = RenderObjectData[invocationIndex];
    if (renderObject.modelIndex < 0)
        return;

    RenderModelAttributes modelAttributes = RenderModelData[renderObject.modelIndex];
    if (modelAttributes.submeshCount == 0 || modelAttributes.firstSubmeshIndex < 0)
        return;

    for(uint i = 0; i < modelAttributes.submeshCount; ++i)
    {
        uint submeshIndex = modelAttributes.firstSubmeshIndex + i;
        RenderMeshPrimitiveAttributes submesh = RenderSubmeshData[submeshIndex];
        if(submesh.vertexCount == 0 || submesh.indexCount == 0 || submesh.firstIndexPosition < 0)
            continue;

        uint workChunkIndex = atomicAdd(renderMeshChunkCount, 1);
        RenderMeshChunk chunk;
        chunk.renderObjectIndex = invocationIndex;
        chunk.renderMeshPrimitiveIndex = submeshIndex;
        RenderMeshChunkData[workChunkIndex] = chunk;
    }
    
}