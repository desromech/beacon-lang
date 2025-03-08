#line 1
layout(local_size_x = 128) in;

void main()
{
    uint invocationIndex = gl_GlobalInvocationID.x;

    DrawIndexedIndirectCommand command;
    command.indexCount = 0;
    command.instanceCount = 0;
    command.firstIndex = 0;
    command.vertexOffset = 0;
    command.firstInstance = invocationIndex;

    if (invocationIndex < renderMeshChunkCount)
    {
        RenderMeshChunk meshChunk = RenderMeshChunkData[invocationIndex];
        RenderMeshPrimitiveAttributes submesh = RenderSubmeshData[meshChunk.renderMeshPrimitiveIndex];
        command.indexCount = submesh.indexCount;
        command.firstIndex = submesh.firstIndexPosition;
        command.instanceCount = 1;
    }
    
    DrawIndexedIndirectData[invocationIndex] = command;
}
