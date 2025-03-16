
#include "AgpuRendering.h"
#include "Exceptions.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stb_truetype.h"

typedef struct VulkanDrawIndexedIndirectCommand {
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    int32_t     vertexOffset;
    uint32_t    firstInstance;
} VulkanDrawIndexedIndirectCommand;

typedef VulkanDrawIndexedIndirectCommand DrawIndirectCommand;

void shadowMapAtlasAllocator_initializeWithExtent(beacon_AGPUShadowMapAtlasAllocator_t *allocator, uint32_t atlasWidth, uint32_t atlasHeight)
{
    allocator->atlasWidth = atlasWidth;
    allocator->atlasHeight = atlasHeight;

    allocator->columns = 4;
    allocator->rows = 4;

    allocator->shadowMapExtent.x = atlasWidth / allocator->columns;
    allocator->shadowMapExtent.y = atlasHeight / allocator->rows;

    allocator->capacity = allocator->columns*allocator->rows;
    allocator->size = 0;
}

void shadowMapAtlasAllocator_reset(beacon_AGPUShadowMapAtlasAllocator_t *allocator)
{
    allocator->size = 0;
}

bool shadowMapAtlasAllocator_allocate(beacon_AGPUShadowMapAtlasAllocator_t *allocator, beacon_AGPUShadowMapAtlasAllocation_t *allocation)
{
    if(allocator->size >= allocator->capacity)
        return false;

    memset(allocation, 0, sizeof(allocation));

    int row = allocator->size / allocator->columns;
    int column = allocator->size % allocator->columns;
    
    allocation->offset.x = column*allocator->shadowMapExtent.x;
    allocation->offset.y = row*allocator->shadowMapExtent.y;
    
    allocation->shadowMapExtent = allocator->shadowMapExtent;

    allocation->shadowMapAtlasExtent.x = allocator->atlasWidth;
    allocation->shadowMapAtlasExtent.y = allocator->atlasHeight;

    ++allocator->size;

    return true;

}


agpu_platform *beacon_agpu_getPlatform(beacon_context_t *context, beacon_AGPU_t *agpu)
{
    if(agpu->platform)
        return agpu->platform;

    // Get the platform.
    agpu_uint numPlatforms;
    agpuGetPlatforms(0, NULL, &numPlatforms);
    if (numPlatforms == 0)
    {
        fprintf(stderr, "No agpu platforms are available.\n");
        return NULL;
    }
    else if(agpu->platformIndex >= (int)numPlatforms)
    {
        fprintf(stderr, "Warning selected AGPU platform is not available. Falling back to the first available\n");
        agpu->platformIndex = 0;
    }

    agpu_platform **platforms = calloc(numPlatforms, sizeof(agpu_platform *));
    agpuGetPlatforms(numPlatforms, platforms, NULL);

    agpu_platform *selectedPlatform = platforms[agpu->platformIndex];
    free(platforms);
    agpu->platform = selectedPlatform;
    printf("Selected AGPU Platform: %s\n", agpuGetPlatformName(selectedPlatform));
    return selectedPlatform;
}

agpu_device *beacon_agpu_getDevice(beacon_context_t *context, beacon_AGPU_t *agpu)
{
    if(agpu->device)
        return agpu->device;

    agpu_platform *platform = beacon_agpu_getPlatform(context, agpu);
    if(!platform)
        return 0;
    
    agpu_device_open_info openInfo = {};
    openInfo.gpu_index = agpu->deviceIndex;
    openInfo.debug_layer = agpu->debugLayerEnabled;

    agpu_device *device = agpuOpenDevice(platform, &openInfo);
    if(!device)
    {
        fprintf(stderr, "Failed to open the specified agpu device.\n");
        return NULL;
    }

    printf("Selected AGPU Device: %s\n", agpuGetDeviceName(device));
    agpu->device = device;
    return device;
}

static agpu_shader *beacon_agpu_compileShaderWithSource(beacon_context_t *context, beacon_AGPU_t *agpu, const char *name, const char *source, agpu_shader_type shaderType)
{
    if(!source)
        return NULL;
    
    agpu_offline_shader_compiler *shaderCompiler = agpuCreateOfflineShaderCompilerForDevice(agpu->device);
    agpuSetOfflineShaderCompilerSource(shaderCompiler, AGPU_SHADER_LANGUAGE_VGLSL, shaderType, source, strlen(source));

    agpu_error errorCode = agpuCompileOfflineShader(shaderCompiler, AGPU_SHADER_LANGUAGE_DEVICE_SHADER, NULL);
    if(errorCode)
    {
        size_t logLength = agpuGetOfflineShaderCompilationLogLength(shaderCompiler);
        char *log = calloc(logLength + 1, 1);
        agpuGetOfflineShaderCompilationLog(shaderCompiler, logLength, log);
        fprintf(stderr, "Compilation error of '%s':%s\n", name, log);
        free(log);
        agpuReleaseOfflineShaderCompiler(shaderCompiler);
        return NULL;
    }

    agpu_shader *shaderResult = agpuGetOfflineShaderCompilerResultAsShader(shaderCompiler);
    agpuReleaseOfflineShaderCompiler(shaderCompiler);
    return shaderResult;
}

static char *beacon_agpu_readShaderSourceFromFileNamed(const char *name, const char *sourceFileName)
{
    FILE *f = fopen(sourceFileName, "rb");
    if(!f)
    {
        fprintf(stderr, "Failed to open shader source file: %s\n", sourceFileName);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *shaderSource = calloc(1, fileSize + 1);
    if(fread(shaderSource, fileSize, 1, f) != 1)
    {
        fprintf(stderr, "Failed to read shader source file: %s\n", sourceFileName);
        fclose(f);
        return NULL;
    }

    fclose(f);
    return shaderSource;
}

static agpu_shader *beacon_agpu_compileShaderWithSourceFileNamed(beacon_context_t *context, beacon_AGPU_t *agpu, const char *name, const char *commonSourceFileName, const char *sourceFileName, agpu_shader_type shaderType)
{
    char *commonSource = NULL;
    if(commonSourceFileName)
        commonSource = beacon_agpu_readShaderSourceFromFileNamed("Common", commonSourceFileName);
    
    char *shaderSource = beacon_agpu_readShaderSourceFromFileNamed("Common", sourceFileName);
    if(!shaderSource)
    {
        if(commonSource)
            free(commonSource);
        return NULL;
    }

    size_t commonSourceSize = commonSource ? strlen(commonSource) : 0;
    size_t shaderSourceSourceSize = shaderSource ? strlen(shaderSource) : 0;
    size_t combinedSourceSize = commonSourceSize + shaderSourceSourceSize;
    char *combinedSource = calloc(1, combinedSourceSize + 1);
    memcpy(combinedSource, commonSource, commonSourceSize);
    memcpy(combinedSource + commonSourceSize, shaderSource, shaderSourceSourceSize);

    agpu_shader *shader = beacon_agpu_compileShaderWithSource(context, agpu, name, combinedSource, shaderType);

    if(commonSource)
        free(commonSource);
    if(shaderSource)
        free(shaderSource);
    if(combinedSource)
        free(combinedSource);
    return shader;
}

static agpu_shader *beacon_agpu_compileShaderWithTwoCommonSources(beacon_context_t *context, beacon_AGPU_t *agpu, const char *name, const char *commonSourceFileName, const char *secondSourceFileName, const char *sourceFileName, agpu_shader_type shaderType)
{
    char *commonSource = NULL;
    if(commonSourceFileName)
        commonSource = beacon_agpu_readShaderSourceFromFileNamed("Common", commonSourceFileName);

    char *secondCommonSource = NULL;
    if(secondSourceFileName)
        secondCommonSource = beacon_agpu_readShaderSourceFromFileNamed("Common", secondSourceFileName);
    
    char *shaderSource = beacon_agpu_readShaderSourceFromFileNamed("Common", sourceFileName);
    if(!shaderSource)
    {
        if(commonSource)
            free(commonSource);
        if(secondCommonSource)
            free(secondCommonSource);
        return NULL;
    }

    size_t commonSourceSize = commonSource ? strlen(commonSource) : 0;
    size_t secondCommonSourceSize = secondCommonSource ? strlen(secondCommonSource) : 0;
    size_t shaderSourceSourceSize = shaderSource ? strlen(shaderSource) : 0;
    size_t combinedSourceSize = commonSourceSize + secondCommonSourceSize + shaderSourceSourceSize;
    char *combinedSource = calloc(1, combinedSourceSize + 1);
    memcpy(combinedSource, commonSource, commonSourceSize);
    memcpy(combinedSource + commonSourceSize, secondCommonSource, secondCommonSourceSize);
    memcpy(combinedSource + commonSourceSize + secondCommonSourceSize, shaderSource, shaderSourceSourceSize);

    agpu_shader *shader = beacon_agpu_compileShaderWithSource(context, agpu, name, combinedSource, shaderType);

    if(commonSource)
        free(commonSource);
    if(secondCommonSource)
        free(secondCommonSource);
    if(shaderSource)
        free(shaderSource);
    if(combinedSource)
        free(combinedSource);
    return shader;
}

void beacon_agpu_loadPipelineStates(beacon_context_t *context, beacon_AGPU_t *agpu)
{
    agpu_device *device = agpu->device;
    bool hasTextureInvertedProjectionY = agpuHasTopLeftNdcOrigin(device) == agpuHasBottomLeftTextureCoordinates(device);

    agpu_shader *screenQuadShader;
    if (hasTextureInvertedProjectionY)
        screenQuadShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "GuiVertex", NULL, "scripts/runtime/assets/shaders/ScreenQuadFlippedY.glsl", AGPU_VERTEX_SHADER);
    else
        screenQuadShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "GuiVertex", NULL, "scripts/runtime/shaders/ScreenQuad.glsl", AGPU_VERTEX_SHADER);

    // Clear depth
    {
        agpu_shader *clearDepthShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "ClearDepth", NULL, "scripts/runtime/shaders/ClearDepth.glsl", AGPU_FRAGMENT_SHADER);
        agpu_pipeline_builder *builder = agpuCreatePipelineBuilder(device);
        agpuSetRenderTargetCount(builder, 0);
        agpuSetDepthStencilFormat(builder, BEACON_AGPU_DEPTH_FORMAT);
        agpuSetPipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachShader(builder, screenQuadShader);
        agpuAttachShader(builder, clearDepthShader);
        agpuSetPrimitiveType(builder, AGPU_TRIANGLES);
        agpuSetDepthState(builder, true, true, AGPU_ALWAYS);
        agpuSetCullMode(builder, AGPU_CULL_MODE_BACK);
        agpu->clearDepthPipeline = agpuBuildPipelineState(builder);
        agpuReleaseShader(clearDepthShader);
        agpuReleasePipelineBuilder(builder);
    }

    // Shadow map vertex
    {
        agpu_shader *shadowMapVertexShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "ShadowMapVertex", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/ShadowMapVertex.glsl", AGPU_VERTEX_SHADER);
        agpu_pipeline_builder *builder = agpuCreatePipelineBuilder(device);
        agpuSetRenderTargetCount(builder, 0);
        agpuSetDepthStencilFormat(builder, BEACON_AGPU_DEPTH_FORMAT);
        agpuSetPipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachShader(builder, shadowMapVertexShader);
        agpuSetPrimitiveType(builder, AGPU_TRIANGLES);
        agpuSetDepthState(builder, true, true, AGPU_GREATER_EQUAL);
        agpuSetDepthBias(builder, -2.0, 0.0, -1.0);
        agpu->shadowMapDepthPipeline = agpuBuildPipelineState(builder);
        agpuReleaseShader(shadowMapVertexShader);
        agpuReleasePipelineBuilder(builder);
    }

    // Depth only
    {
        agpu_shader *depthOnlyVertexShaders = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "DepthOnlyVertex", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/DepthOnlyVertex.glsl", AGPU_VERTEX_SHADER);
        agpu_pipeline_builder *builder = agpuCreatePipelineBuilder(device);
        agpuSetRenderTargetCount(builder, 0);
        agpuSetDepthStencilFormat(builder, BEACON_AGPU_DEPTH_FORMAT);
        agpuSetPipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachShader(builder, depthOnlyVertexShaders);
        agpuSetPrimitiveType(builder, AGPU_TRIANGLES);
        agpuSetDepthState(builder, true, true, AGPU_GREATER_EQUAL);
        agpuSetCullMode(builder, AGPU_CULL_MODE_BACK);
        agpu->opaqueDepthOnlyPipeline = agpuBuildPipelineState(builder);
        agpuReleaseShader(depthOnlyVertexShaders);
        agpuReleasePipelineBuilder(builder);
    }

    {
        agpu_shader *daySkyShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "DaySkyShader", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/DaySkyShader.glsl", AGPU_FRAGMENT_SHADER);
        agpu_pipeline_builder *builder = agpuCreatePipelineBuilder(device);
        agpuSetRenderTargetCount(builder, 3);
        agpuSetRenderTargetFormat(builder, 0, BEACON_AGPU_COLOR_FORMAT);
        agpuSetRenderTargetFormat(builder, 1, AGPU_TEXTURE_FORMAT_R16G16_FLOAT);
        agpuSetRenderTargetFormat(builder, 2, AGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM);
        agpuSetDepthStencilFormat(builder, BEACON_AGPU_DEPTH_FORMAT);
        agpuSetPipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachShader(builder, screenQuadShader);
        agpuAttachShader(builder, daySkyShader);
        agpuSetPrimitiveType(builder, AGPU_TRIANGLE_STRIP);
        agpuSetDepthState(builder, true, true, AGPU_EQUAL);
        agpuSetCullMode(builder, AGPU_CULL_MODE_BACK);
        agpu->daySkyPipeline = agpuBuildPipelineState(builder);
        agpuReleaseShader(daySkyShader);
        agpuReleasePipelineBuilder(builder);
    }

    {
        agpu_shader *toneMapping = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "ToneMapping", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/FilmicTonemapping.glsl", AGPU_FRAGMENT_SHADER);
        agpu_pipeline_builder *builder = agpuCreatePipelineBuilder(device);
        agpuSetRenderTargetFormat(builder, 0, BEACON_AGPU_SWAP_CHAIN_COLOR_FORMAT);
        agpuSetDepthStencilFormat(builder, AGPU_TEXTURE_FORMAT_UNKNOWN);
        agpuSetPipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachShader(builder, screenQuadShader);
        agpuAttachShader(builder, toneMapping);
        agpuSetPrimitiveType(builder, AGPU_TRIANGLE_STRIP);
        agpuSetCullMode(builder, AGPU_CULL_MODE_BACK);
        agpu->toneMappingPipeline = agpuBuildPipelineState(builder);
        agpuReleaseShader(toneMapping);
        agpuReleasePipelineBuilder(builder);
    }

    {
        agpu_shader *cullOpaqueShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "CullOpaque", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/CullOpaqueObjects.glsl", AGPU_COMPUTE_SHADER);
        agpu_compute_pipeline_builder *builder = agpuCreateComputePipelineBuilder(device);
        agpuSetComputePipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachComputeShader(builder, cullOpaqueShader);
        agpu->cullOpaqueObjects = agpuBuildComputePipelineState(builder);
        agpuReleaseShader(cullOpaqueShader);
        agpuReleaseComputePipelineBuilder(builder);
    }

    {
        agpu_shader *clearRenderChunkDataShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "ClearRenderChunk", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/ClearRenderChunkData.glsl", AGPU_COMPUTE_SHADER);
        agpu_compute_pipeline_builder *builder = agpuCreateComputePipelineBuilder(device);
        agpuSetComputePipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachComputeShader(builder, clearRenderChunkDataShader);
        agpu->clearRenderChunkData = agpuBuildComputePipelineState(builder);
        agpuReleaseShader(clearRenderChunkDataShader);
        agpuReleaseComputePipelineBuilder(builder);
    }

    {
        agpu_shader *makeIndirectDraw = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "MakeDrawIndirect", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/MakeDrawIndirectCommands.glsl", AGPU_COMPUTE_SHADER);
        agpu_compute_pipeline_builder *builder = agpuCreateComputePipelineBuilder(device);
        agpuSetComputePipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachComputeShader(builder, makeIndirectDraw);
        agpu->makeDrawIndirectPipeline = agpuBuildComputePipelineState(builder);
        agpuReleaseShader(makeIndirectDraw);
        agpuReleaseComputePipelineBuilder(builder);
    }

    {
        agpu_shader *transformLightsToShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "TransformLightsToView", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/TransformLightsToView.glsl", AGPU_COMPUTE_SHADER);
        agpu_compute_pipeline_builder *builder = agpuCreateComputePipelineBuilder(device);
        agpuSetComputePipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachComputeShader(builder, transformLightsToShader);
        agpu->transformLightsToViewPipeline = agpuBuildComputePipelineState(builder);
        agpuReleaseShader(transformLightsToShader);
        agpuReleaseComputePipelineBuilder(builder);
    }
    
    {
        agpu_shader *lightGridComputationShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "LightGridComputation", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/LightGridComputation.glsl", AGPU_COMPUTE_SHADER);
        agpu_compute_pipeline_builder *builder = agpuCreateComputePipelineBuilder(device);
        agpuSetComputePipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachComputeShader(builder, lightGridComputationShader);
        agpu->lightGridComputationPipeline = agpuBuildComputePipelineState(builder);
        agpuReleaseShader(lightGridComputationShader);
        agpuReleaseComputePipelineBuilder(builder);
    }

    {
        agpu_shader *lightClusterBeginShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "LightClusterBeginComputation", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/LightClusterBeginComputation.glsl", AGPU_COMPUTE_SHADER);
        agpu_compute_pipeline_builder *builder = agpuCreateComputePipelineBuilder(device);
        agpuSetComputePipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachComputeShader(builder, lightClusterBeginShader);
        agpu->lightClusterBeginComputationPipeline = agpuBuildComputePipelineState(builder);
        agpuReleaseShader(lightClusterBeginShader);
        agpuReleaseComputePipelineBuilder(builder);
    }

    {
        agpu_shader *lightClusterListComputationShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "LightClusterListComputation", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/LightClusterListComputation.glsl", AGPU_COMPUTE_SHADER);
        agpu_compute_pipeline_builder *builder = agpuCreateComputePipelineBuilder(device);
        agpuSetComputePipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachComputeShader(builder, lightClusterListComputationShader);
        agpu->lightClusterListComputationPipeline = agpuBuildComputePipelineState(builder);
        agpuReleaseShader(lightClusterListComputationShader);
        agpuReleaseComputePipelineBuilder(builder);
    }

    //beacon_agpu_readShaderSourceFromFileNamed
    {
        
        agpu_shader *opaqueVertexShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "OpaqueColorVertex", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/GenericVertex.glsl", AGPU_VERTEX_SHADER);
        agpu_shader *opaqueFragmentShader = beacon_agpu_compileShaderWithTwoCommonSources(context, agpu, "OpaqueColorFragment", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/ShaderFragmentCommon.glsl", "scripts/runtime/shaders/OpaqueFragment.glsl", AGPU_FRAGMENT_SHADER);
    
        agpu_pipeline_builder *builder = agpuCreatePipelineBuilder(device);
        agpuSetRenderTargetCount(builder, 3);
        agpuSetRenderTargetFormat(builder, 0, BEACON_AGPU_COLOR_FORMAT);
        agpuSetRenderTargetFormat(builder, 1, AGPU_TEXTURE_FORMAT_R16G16_FLOAT);
        agpuSetRenderTargetFormat(builder, 2, AGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM);
        agpuSetDepthStencilFormat(builder, BEACON_AGPU_DEPTH_FORMAT);
        agpuSetPipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachShader(builder, opaqueVertexShader);
        agpuAttachShader(builder, opaqueFragmentShader);
        agpuSetPrimitiveType(builder, AGPU_TRIANGLES);
        agpuSetDepthState(builder, true, true, AGPU_EQUAL);
        agpuSetCullMode(builder, AGPU_CULL_MODE_BACK);
        agpu->opaqueColorPipeline = agpuBuildPipelineState(builder);
        
        agpuReleaseShader(opaqueVertexShader);
        agpuReleaseShader(opaqueFragmentShader);
        agpuReleasePipelineBuilder(builder);
    }
    // Uber GUI pipeline state
    {
        //printf("guiVertexShaderSource: %s\n", guiVertexShaderSource);
        //printf("guiFragmentShaderSource: %s\n", guiFragmentShaderSource);

        agpu_shader *vertexShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "GuiVertex", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/GuiVertexShader.glsl", AGPU_VERTEX_SHADER);
        agpu_shader *fragmentShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "GuiFragment", "scripts/runtime/shaders/ShaderCommon.glsl", "scripts/runtime/shaders/GuiFragmentShader.glsl", AGPU_FRAGMENT_SHADER);

        agpu_pipeline_builder *builder = agpuCreatePipelineBuilder(agpu->device);
        agpuSetRenderTargetFormat(builder, 0, AGPU_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB);
        agpuSetDepthStencilFormat(builder, AGPU_TEXTURE_FORMAT_UNKNOWN);
        agpuSetPipelineShaderSignature(builder, agpu->shaderSignature);
        agpuAttachShader(builder, vertexShader);
        agpuAttachShader(builder, fragmentShader);
        agpuSetPrimitiveType(builder, AGPU_TRIANGLE_STRIP);
        agpuSetBlendState(builder, -1, true);
        agpuSetBlendFunction(builder, -1, AGPU_BLENDING_ONE, AGPU_BLENDING_INVERTED_SRC_ALPHA, AGPU_BLENDING_OPERATION_ADD,
            AGPU_BLENDING_ONE, AGPU_BLENDING_INVERTED_SRC_ALPHA, AGPU_BLENDING_OPERATION_ADD);
        agpu->guiPipelineState = agpuBuildPipelineState(builder);
        if(!agpu->guiPipelineState)
        {
            fprintf(stderr, "Failed to construct pipeline state.\n");
        }

        agpuReleaseShader(vertexShader);
        agpuReleaseShader(fragmentShader);
    }

    agpuReleaseShader(screenQuadShader);
}

void beacon_agpu_initializeCommonObjects(beacon_context_t *context, beacon_AGPU_t *agpu)
{
    if(agpu->shaderSignature)
        return;

    // Main render pass
    {
        agpu_renderpass_color_attachment_description colorAttachment = {
            .format = AGPU_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB,
            .begin_action = AGPU_ATTACHMENT_CLEAR,
            .end_action = AGPU_ATTACHMENT_KEEP,
            .clear_value = {
                .r = 0, .g = 0, .b = 0, .a = 1
            },
        };
        agpu_renderpass_description description = {
            .color_attachment_count = 1,
            .color_attachments = &colorAttachment
        };

        agpu->mainRenderPass = agpuCreateRenderPass(agpu->device, &description);
    }

    // Shader signature
    {
        agpu_shader_signature_builder *builder = agpuCreateShaderSignatureBuilder(agpu->device);
        
        agpuBeginShaderSignatureBindingBank(builder, 1); // Set 0
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_SAMPLER, 1); // Linear
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_SAMPLER, 1); // Nearest
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_SAMPLER, 1); // ShadowMap

        agpuBeginShaderSignatureBindingBank(builder, 1); // Set 1
        agpuAddShaderSignatureBindingBankArray(builder, AGPU_SHADER_BINDING_TYPE_SAMPLED_IMAGE, BEACON_AGPU_TEXTURE_ARRAY_SIZE);

        agpuBeginShaderSignatureBindingBank(builder, 1); // Set 2

        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 0: Render object
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 1: Render model
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 2: Render submesh
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 3: Render material
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 4: Render light source

        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 5: Vertex Positions
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 6: Vertex Normals
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 7: Vertex Texcoords
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 8: Vertex Tangent4
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 9: Vertex BoneIndices
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 10: Vertex BoneWeights

        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 11: Draw indirect
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 12: Mesh chunks

        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 13: View space lights
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 14: LightClustersBlock
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 15: TileLightIndicesBlock
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 16: LightClusterListsBlock
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_SAMPLED_IMAGE, 1);  // 17: Shadow map atlas

        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 18: Gui elements

        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1); // 19: Camera state

        agpuBeginShaderSignatureBindingBank(builder, 1024); // Set 3 - Postprocessing
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_SAMPLED_IMAGE, 1); // HdrColorBuffer

        agpuAddShaderSignatureBindingConstant(builder); // renderObjectsSize
        agpuAddShaderSignatureBindingConstant(builder); // lightSourceCount
        agpuAddShaderSignatureBindingConstant(builder); // shadowMapLightSourceIndex
        agpuAddShaderSignatureBindingConstant(builder); // shadowMapComponent

        agpuAddShaderSignatureBindingConstant(builder); // hasTopLeftNDCOrigin
        agpuAddShaderSignatureBindingConstant(builder); // reservedConstant
        agpuAddShaderSignatureBindingConstant(builder); // framebufferReciprocalExtentX
        agpuAddShaderSignatureBindingConstant(builder); // framebufferReciprocalExtentY

        agpu->shaderSignature = agpuBuildShaderSignature(builder);
    }

    // Sampler and its binding
    agpu->samplerBinding = agpuCreateShaderResourceBinding(agpu->shaderSignature, 0);
    {
        agpu_sampler_description samplerDesc = {
            .address_u = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .address_v = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .address_w = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .filter = AGPU_FILTER_MIN_LINEAR_MAG_LINEAR_MIPMAP_NEAREST,
            .max_lod = 32
        };

        agpu->linearSampler = agpuCreateSampler(agpu->device, &samplerDesc);
        agpuBindSampler(agpu->samplerBinding, 0, agpu->linearSampler);
    }

    {
        agpu_sampler_description samplerDesc = {
            .address_u = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .address_v = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .address_w = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .filter = AGPU_FILTER_MIN_NEAREST_MAG_NEAREST_MIPMAP_NEAREST,
            .max_lod = 32
        };

        agpu->nearestSampler = agpuCreateSampler(agpu->device, &samplerDesc);
        agpuBindSampler(agpu->samplerBinding, 1, agpu->nearestSampler);
    }

    {
        agpu_sampler_description samplerDesc = {
            .address_u = AGPU_TEXTURE_ADDRESS_MODE_CLAMP,
            .address_v = AGPU_TEXTURE_ADDRESS_MODE_CLAMP,
            .address_w = AGPU_TEXTURE_ADDRESS_MODE_CLAMP,
            .comparison_enabled = true,
            .comparison_function = AGPU_GREATER_EQUAL,
            .max_lod = 0.0,

            .filter = AGPU_FILTER_MIN_LINEAR_MAG_LINEAR_MIPMAP_NEAREST,
        };

        agpu->shadowSampler = agpuCreateSampler(agpu->device, &samplerDesc);
        agpuBindSampler(agpu->samplerBinding, 2, agpu->shadowSampler);
    }

    // Error texture
    {
        uint32_t m = 0xFF00FFFF;
        uint32_t colors[] = {
            0, 0, m, m, 0, 0, m, m,
            0, 0, m, m, 0, 0, m, m,
            m, m, 0, 0, m, m, 0, 0,
            m, m, 0, 0, m, m, 0, 0,
            0, 0, m, m, 0, 0, m, m,
            0, 0, m, m, 0, 0, m, m,
            m, m, 0, 0, m, m, 0, 0,
            m, m, 0, 0, m, m, 0, 0,
        };

        agpu_texture_description desc = {
            .type = AGPU_TEXTURE_2D,
            .width = 8,
            .height = 8,
            .depth = 1,
            .layers = 1,
            .miplevels = 1,
            .format = AGPU_TEXTURE_FORMAT_B8G8R8A8_UNORM,
            .usage_modes = AGPU_TEXTURE_USAGE_COPY_DESTINATION | AGPU_TEXTURE_USAGE_SAMPLED,
            .main_usage_mode = AGPU_TEXTURE_USAGE_SAMPLED,
            .heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL,
            .sample_count = 1,
            .sample_quality = 0,
        };

        agpu->errorTexture = agpuCreateTexture(agpu->device, &desc);
        agpuUploadTextureData(agpu->errorTexture, 0, 0, 8*4, 8*8*4, colors);

    }
    // Textures bindings
    {
        agpu_sampler_description samplerDesc = {
            .address_u = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .address_v = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .address_w = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .filter = AGPU_FILTER_MIN_LINEAR_MAG_LINEAR_MIPMAP_NEAREST,
            .max_lod = 32
        };

        agpu->textureArrayBindingCount = 0;
        agpu->texturesArrayBinding = agpuCreateShaderResourceBinding(agpu->shaderSignature, 1);
        agpu_texture_view *errorTextureView = agpuGetOrCreateFullTextureView(agpu->errorTexture);

        for(size_t i = 0; i < BEACON_AGPU_TEXTURE_ARRAY_SIZE; ++i)
        {
            agpu->boundTextures[i] = agpu->errorTexture;
            agpu->boundTextureViews[i] = errorTextureView;
            agpuBindArrayOfSampledTextureView(agpu->texturesArrayBinding, 0, i, 1, &errorTextureView);
        }
        agpu->textureArrayBindingCount = 1;
    }

    // Shadow map atlas
    {
        agpu_texture_description desc = {
            .type = AGPU_TEXTURE_2D,
            .width = BEACON_AGPU_SHADOW_MAP_ATLAS_SIZE,
            .height = BEACON_AGPU_SHADOW_MAP_ATLAS_SIZE,
            .depth = 1,
            .layers = 1,
            .miplevels = 1,
            .format = AGPU_TEXTURE_FORMAT_D32_FLOAT,
            .usage_modes = AGPU_TEXTURE_USAGE_COPY_DESTINATION | AGPU_TEXTURE_USAGE_DEPTH_ATTACHMENT | AGPU_TEXTURE_USAGE_SAMPLED,
            .main_usage_mode = AGPU_TEXTURE_USAGE_SAMPLED,
            .heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL,
            .sample_count = 1,
            .sample_quality = 0,
        };

        agpu->shadowMapAtlas = agpuCreateTexture(agpu->device, &desc);
        agpu->shadowMapFramebuffer = agpuCreateFrameBuffer(agpu->device, BEACON_AGPU_SHADOW_MAP_ATLAS_SIZE, BEACON_AGPU_SHADOW_MAP_ATLAS_SIZE,0, NULL, agpuGetOrCreateFullTextureView(agpu->shadowMapAtlas));
        shadowMapAtlasAllocator_initializeWithExtent(&agpu->shadowMapAtlasAllocator, desc.width, desc.height);
    }

    beacon_agpu_loadPipelineStates(context, agpu);
}

beacon_AGPUTextureHandle_t *beacon_getValidTextureHandleForFontFaceForm(beacon_context_t *context, beacon_Form_t *form)
{
    if(!form->textureHandle)
    {
        BeaconAssert(context, beacon_decodeSmallInteger(form->depth) == 8);
        agpu_texture_description desc = {
            .type = AGPU_TEXTURE_2D,
            .width = beacon_decodeSmallInteger(form->width),
            .height = beacon_decodeSmallInteger(form->height),
            .depth = 1,
            .layers = 1,
            .miplevels = 1,
            .format = AGPU_TEXTURE_FORMAT_R8_UNORM,
            .usage_modes = AGPU_TEXTURE_USAGE_COPY_DESTINATION | AGPU_TEXTURE_USAGE_SAMPLED,
            .main_usage_mode = AGPU_TEXTURE_USAGE_SAMPLED,
            .heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL,
            .sample_count = 1,
            .sample_quality = 0,
        };

        agpu_texture *texture = agpuCreateTexture(context->roots.agpuCommon->device, &desc);
        if(!texture)
            return NULL;

        agpuUploadTextureData(texture, 0, 0, beacon_decodeSmallInteger(form->pitch), beacon_decodeSmallInteger(form->pitch)*beacon_decodeSmallInteger(form->height), form->bits->elements);
        beacon_AGPUTextureHandle_t *handle = beacon_allocateObjectWithBehavior(context->heap, context->classes.agpuTextureHandleClass, sizeof(beacon_AGPUTextureHandle_t), BeaconObjectKindBytes);

        handle->texture = texture;
        handle->textureView = agpuGetOrCreateFullTextureView(texture);
        handle->textureArrayBindingIndex = context->roots.agpuCommon->textureArrayBindingCount;

        agpuBindArrayOfSampledTextureView(context->roots.agpuCommon->texturesArrayBinding, 0, handle->textureArrayBindingIndex, 1, &handle->textureView);

        ++context->roots.agpuCommon->textureArrayBindingCount;
        form->textureHandle = (beacon_oop_t)handle;
    }

    return (beacon_AGPUTextureHandle_t *)form->textureHandle;
}

static size_t initializeUpdateBuffer(beacon_AGPUUpdateBuffer_t *buffer, beacon_AGPUUpdateBuffer_t *previous, size_t elementSize, size_t capacity)
{
    beacon_AGPUUpdateBuffer_t updateBuffer = {
        .offset = previous ? previous->endOffset : 0, 
        .elementSize = elementSize,
        .capacity = capacity,
        .byteCapacity = elementSize *capacity,
        .size = 0,
        .endOffset = (previous ? previous->endOffset : 0) + elementSize * capacity
    };

    *buffer = updateBuffer;
    return updateBuffer.endOffset;
}

void beacon_agpu_initializeUpdateBuffers(beacon_context_t *context, beacon_AGPU_t *agpu)
{
    initializeUpdateBuffer(&agpu->renderObjectAttributes, NULL, sizeof(beacon_RenderObjectAttributes_t), BEACON_AGPU_MAX_RENDER_OBJECTS);
    initializeUpdateBuffer(&agpu->renderModelAttributes, &agpu->renderObjectAttributes, sizeof(beacon_RenderObjectAttributes_t), BEACON_AGPU_MAX_MODELS);
    initializeUpdateBuffer(&agpu->renderMeshPrimitiveAttributes, &agpu->renderModelAttributes, sizeof(beacon_RenderMeshPrimitiveAttributes_t), BEACON_AGPU_MAX_MESHES);
    initializeUpdateBuffer(&agpu->renderMaterialsAttributes, &agpu->renderMeshPrimitiveAttributes, sizeof(beacon_RenderMaterialAttributes_t), BEACON_AGPU_MAX_MATERIALS);
    initializeUpdateBuffer(&agpu->renderLightSourceAttributes, &agpu->renderMaterialsAttributes, sizeof(beacon_RenderLightSource_t), BEACON_AGPU_MAX_MATERIALS);

    initializeUpdateBuffer(&agpu->vertexPositions,   &agpu->renderLightSourceAttributes, sizeof(beacon_RenderPackedVector3_t), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexNormals,     &agpu->vertexPositions,             sizeof(beacon_RenderPackedVector3_t), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexTexcoords,   &agpu->vertexNormals,               sizeof(beacon_RenderVector2_t), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexTangent4,    &agpu->vertexTexcoords,             sizeof(beacon_RenderVector4_t), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexBoneIndices, &agpu->vertexTangent4,              4*sizeof(uint16_t), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexBoneWeights, &agpu->vertexBoneIndices,           sizeof(beacon_RenderVector4_t), BEACON_AGPU_MAX_VERTICES);

    initializeUpdateBuffer(&agpu->guiData, &agpu->vertexBoneWeights, sizeof(beacon_GuiRenderingElement_t), BEACON_AGPU_MAX_NUMBER_OF_QUADS);

    initializeUpdateBuffer(&agpu->cameraState, &agpu->guiData, sizeof(beacon_RenderCameraState_t), BEACON_AGPU_MAX_CAMERA_STATES);

    initializeUpdateBuffer(&agpu->indexData, &agpu->cameraState, sizeof(uint32_t), BEACON_AGPU_MAX_INDICES);

    agpu_device *device = agpu->device;
    {
        agpu_buffer_description desc = {
            .heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL,
            .usage_modes = AGPU_COPY_DESTINATION_BUFFER | AGPU_STORAGE_BUFFER,
            .main_usage_mode = AGPU_STORAGE_BUFFER,
            .size = agpu->cameraState.endOffset,
        };

        agpu->gpu3DRenderingDataBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    {
        agpu_buffer_description desc = {
            .heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL,
            .usage_modes = AGPU_COPY_DESTINATION_BUFFER | AGPU_ELEMENT_ARRAY_BUFFER,
            .main_usage_mode = AGPU_ELEMENT_ARRAY_BUFFER,
            .size = agpu->indexData.byteCapacity,
            .stride = 4,
        };

        agpu->gpu3DRenderingIndexBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    {
        agpu_buffer_description desc = {};
        desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
        desc.usage_modes = AGPU_STORAGE_BUFFER | AGPU_DRAW_INDIRECT_BUFFER;
        desc.main_usage_mode = AGPU_DRAW_INDIRECT_BUFFER;
        desc.size = sizeof(DrawIndirectCommand) * BEACON_AGPU_MAX_RENDER_OBJECTS;
        desc.stride = sizeof(DrawIndirectCommand);
        agpu->renderDrawIndirectBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    {
        agpu_buffer_description desc = {};
        desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
        desc.usage_modes = AGPU_STORAGE_BUFFER;
        desc.main_usage_mode = AGPU_STORAGE_BUFFER;
        desc.size = (sizeof(beacon_RenderMeshChunk_t) + 1) * BEACON_AGPU_MAX_RENDER_OBJECTS;
        agpu->renderChunkDataBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    {
        agpu_buffer_description desc = {};
        desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
        desc.usage_modes = AGPU_STORAGE_BUFFER;
        desc.main_usage_mode = AGPU_STORAGE_BUFFER;
        desc.size = sizeof(beacon_RenderLightSource_t) * BEACON_AGPU_MAX_LIGHT_SOURCES;
        agpu->viewLightSourceBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    {
        agpu_buffer_description desc = {};
        desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
        desc.usage_modes = AGPU_STORAGE_BUFFER;
        desc.main_usage_mode = AGPU_STORAGE_BUFFER;
        desc.size = 32 * BEACON_AGPU_LIGHT_GRID_CELL_COUNT;
        agpu->lightClusterBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    {
        agpu_buffer_description desc = {};
        desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
        desc.usage_modes = AGPU_STORAGE_BUFFER;
        desc.main_usage_mode = AGPU_STORAGE_BUFFER;
        desc.size = 4 * BEACON_AGPU_MAX_LIGHT_CLUSTER_CAPACITY * BEACON_AGPU_LIGHT_GRID_CELL_COUNT;
        agpu->tileLightIndexListBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    {
        agpu_buffer_description desc = {};
        desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
        desc.usage_modes = AGPU_STORAGE_BUFFER;
        desc.main_usage_mode = AGPU_STORAGE_BUFFER;
        desc.size = 8 + 8 * BEACON_AGPU_MAX_LIGHT_CLUSTER_CAPACITY * BEACON_AGPU_LIGHT_GRID_CELL_COUNT;
        agpu->lightGridBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    agpu->renderingDataBinding = agpuCreateShaderResourceBinding(context->roots.agpuCommon->shaderSignature, 2);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 0, agpu->gpu3DRenderingDataBuffer, agpu->renderObjectAttributes.offset,        agpu->renderObjectAttributes.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 1, agpu->gpu3DRenderingDataBuffer, agpu->renderModelAttributes.offset,         agpu->renderModelAttributes.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 2, agpu->gpu3DRenderingDataBuffer, agpu->renderMeshPrimitiveAttributes.offset, agpu->renderMeshPrimitiveAttributes.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 3, agpu->gpu3DRenderingDataBuffer, agpu->renderMaterialsAttributes.offset,     agpu->renderMaterialsAttributes.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 4, agpu->gpu3DRenderingDataBuffer, agpu->renderLightSourceAttributes.offset,   agpu->renderLightSourceAttributes.byteCapacity);

    agpuBindStorageBufferRange(agpu->renderingDataBinding, 5,  agpu->gpu3DRenderingDataBuffer, agpu->vertexPositions.offset, agpu->vertexPositions.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 6,  agpu->gpu3DRenderingDataBuffer, agpu->vertexNormals.offset, agpu->vertexNormals.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 7,  agpu->gpu3DRenderingDataBuffer, agpu->vertexTexcoords.offset, agpu->vertexTexcoords.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 8,  agpu->gpu3DRenderingDataBuffer, agpu->vertexTangent4.offset, agpu->vertexTangent4.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 9,  agpu->gpu3DRenderingDataBuffer, agpu->vertexBoneIndices.offset, agpu->vertexBoneIndices.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 10, agpu->gpu3DRenderingDataBuffer, agpu->vertexBoneWeights.offset, agpu->vertexBoneWeights.byteCapacity);

    agpuBindStorageBuffer(agpu->renderingDataBinding, 11, agpu->renderDrawIndirectBuffer);
    agpuBindStorageBuffer(agpu->renderingDataBinding, 12, agpu->renderChunkDataBuffer);

    agpuBindStorageBuffer(agpu->renderingDataBinding, 13, agpu->viewLightSourceBuffer);
    agpuBindStorageBuffer(agpu->renderingDataBinding, 14, agpu->lightClusterBuffer);
    agpuBindStorageBuffer(agpu->renderingDataBinding, 15, agpu->tileLightIndexListBuffer);
    agpuBindStorageBuffer(agpu->renderingDataBinding, 16, agpu->lightGridBuffer);

    agpuBindSampledTextureView(agpu->renderingDataBinding, 17, agpuGetOrCreateFullTextureView(agpu->shadowMapAtlas));

    agpuBindStorageBufferRange(agpu->renderingDataBinding, 18, agpu->gpu3DRenderingDataBuffer, agpu->guiData.offset, agpu->guiData.byteCapacity);
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 19, agpu->gpu3DRenderingDataBuffer, agpu->cameraState.offset, agpu->cameraState.byteCapacity);
}

beacon_AGPUWindowRenderer_t *beacon_agpu_createWindowRenderer(beacon_context_t *context)
{
    agpu_device *device = beacon_agpu_getDevice(context, context->roots.agpuCommon);
    if(!device)
        return NULL;

    if(!context->roots.agpuCommon->shaderSignature)
        beacon_agpu_initializeCommonObjects(context, context->roots.agpuCommon);
    if(!context->roots.agpuCommon->updateBuffersCreated)
        beacon_agpu_initializeUpdateBuffers(context, context->roots.agpuCommon);

    beacon_AGPUWindowRenderer_t *windowRenderer = beacon_allocateObjectWithBehavior(context->heap, context->classes.agpuWindowRendererClass, sizeof(beacon_AGPUWindowRenderer_t), BeaconObjectKindBytes);
    windowRenderer->commandQueue = agpuGetDefaultCommandQueue(device);
    size_t frameRenderingDataSize = context->roots.agpuCommon->indexData.endOffset;
    windowRenderer->frameRenderingDataUploadSize = frameRenderingDataSize;
    
    {
        agpu_buffer_description desc = {
            .heap_type = AGPU_MEMORY_HEAP_TYPE_HOST,
            .usage_modes = AGPU_COPY_SOURCE_BUFFER,
            .main_usage_mode = AGPU_COPY_SOURCE_BUFFER,
            .size = frameRenderingDataSize * BEACON_AGPU_FRAMEBUFFERING_COUNT,
            .mapping_flags = AGPU_MAP_WRITE_BIT | AGPU_MAP_PERSISTENT_BIT
        };
        windowRenderer->renderingDataSubmissionBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    windowRenderer->renderingDataUploadBuffer = agpuMapBuffer(windowRenderer->renderingDataSubmissionBuffer, AGPU_WRITE_ONLY);

    for(int i = 0; i < BEACON_AGPU_FRAMEBUFFERING_COUNT; ++i)
    {
        beacon_AGPUWindowRendererPerFrameState_t *frameState = windowRenderer->frameState + i;
        frameState->fence = agpuCreateFence(device);
        frameState->commandAllocator = agpuCreateCommandAllocator(device, AGPU_COMMAND_LIST_TYPE_DIRECT, windowRenderer->commandQueue);

        frameState->commandList = agpuCreateCommandList(device, AGPU_COMMAND_LIST_TYPE_DIRECT, windowRenderer->frameState[i].commandAllocator, NULL);
        agpuCloseCommandList(frameState->commandList);
    }

    return windowRenderer;
}

void beacon_agpu_destroyWindowRenderer(beacon_context_t *context, beacon_AGPUWindowRenderer_t *renderer)
{
    agpuReleaseCommandQueue(renderer->commandQueue);
    for(int i = 0; i < BEACON_AGPU_FRAMEBUFFERING_COUNT; ++i)
    {
        agpuReleaseFenceReference(renderer->frameState[i].fence);
        agpuReleaseCommandAllocator(renderer->frameState[i].commandAllocator);
        agpuReleaseCommandList(renderer->frameState[i].commandList);
    }
}

static void beacon_resetUpdateBufferPointer(beacon_AGPUUpdateBuffer_t *buffer, uint8_t *frameBasePointer)
{
    buffer->size = 0;
    buffer->thisFrameBuffer = frameBasePointer + buffer->offset;
}

static void beacon_agpuWindowRenderer_resetPerFrameBuffers(beacon_context_t *context, beacon_AGPUWindowRenderer_t *renderer, beacon_AGPUWindowRendererPerFrameState_t *thisFrameState)
{
    uint8_t *renderingDataUploadBuffer = renderer->renderingDataUploadBuffer + renderer->frameRenderingDataUploadSize* renderer->currentFrameBufferingIndex;
    
    beacon_AGPU_t *agpu = context->roots.agpuCommon;
    beacon_resetUpdateBufferPointer(&agpu->renderObjectAttributes, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->renderModelAttributes, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->renderMeshPrimitiveAttributes, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->renderMaterialsAttributes, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->renderLightSourceAttributes, renderingDataUploadBuffer);

    beacon_resetUpdateBufferPointer(&agpu->vertexPositions, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->vertexNormals, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->vertexTexcoords, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->vertexTangent4, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->vertexBoneIndices, renderingDataUploadBuffer);
    beacon_resetUpdateBufferPointer(&agpu->vertexBoneWeights, renderingDataUploadBuffer);

    beacon_resetUpdateBufferPointer(&agpu->guiData, renderingDataUploadBuffer);

    beacon_resetUpdateBufferPointer(&agpu->cameraState, renderingDataUploadBuffer);

    beacon_resetUpdateBufferPointer(&agpu->indexData, renderingDataUploadBuffer);
}

static beacon_oop_t beacon_agpuWindowRenderer_beginFrame(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t*)receiver;
    beacon_AGPUWindowRendererPerFrameState_t *thisFrameState = renderer->frameState + renderer->currentFrameBufferingIndex;
    ++renderer->renderingFrameIndex;

    if(thisFrameState->hasSubmittedToQueue)
    {
        agpuWaitOnClient(thisFrameState->fence);
        thisFrameState->hasSubmittedToQueue = false;
    }

    agpuResetCommandAllocator(thisFrameState->commandAllocator);
    agpuResetCommandList(thisFrameState->commandList, thisFrameState->commandAllocator, NULL);

    uint8_t *renderingDataUploadBuffer = renderer->renderingDataUploadBuffer + renderer->frameRenderingDataUploadSize* renderer->currentFrameBufferingIndex;
    beacon_agpuWindowRenderer_resetPerFrameBuffers(context, renderer, thisFrameState);
    shadowMapAtlasAllocator_reset(&context->roots.agpuCommon->shadowMapAtlasAllocator);
    context->roots.agpuCommon->numberOfShadowCastingLightSources = 0;
    return receiver;
}

static beacon_oop_t beacon_agpuWindowRenderer_begin3DFrameRendering(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 2);
    int displayWidth = beacon_decodeSmallInteger(arguments[0]);
    int displayHeight = beacon_decodeSmallInteger(arguments[1]);
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t*)receiver;
    agpu_device *device = context->roots.agpuCommon->device;

    // Create the render pass
    if(!renderer->mainDepthRenderPass)
    {
        agpu_renderpass_depth_stencil_description depthAttachment = {};
        depthAttachment.format = BEACON_AGPU_DEPTH_FORMAT;
        depthAttachment.begin_action = AGPU_ATTACHMENT_CLEAR;
        depthAttachment.end_action = AGPU_ATTACHMENT_KEEP;
        depthAttachment.clear_value.depth = 0.0;
        depthAttachment.sample_count = 1;

        agpu_renderpass_description description = {};
        description.depth_stencil_attachment = &depthAttachment;

        renderer->mainDepthRenderPass = agpuCreateRenderPass(device, &description);
    }

    if(!renderer->shadowMapAtlasRenderPass)
    {
        agpu_renderpass_depth_stencil_description depthAttachment = {};
        depthAttachment.format = BEACON_AGPU_DEPTH_FORMAT;
        depthAttachment.begin_action = AGPU_ATTACHMENT_KEEP;
        depthAttachment.end_action = AGPU_ATTACHMENT_KEEP;
        depthAttachment.clear_value.depth = 0.0;
        depthAttachment.sample_count = 1;

        agpu_renderpass_description description = {};
        description.depth_stencil_attachment = &depthAttachment;

        renderer->shadowMapAtlasRenderPass = agpuCreateRenderPass(device, &description);
    }

    if(!renderer->mainDepthColorOpaqueRenderPass)
    {
        agpu_renderpass_color_attachment_description colorAttachments[3] = {};

        colorAttachments[0].format = BEACON_AGPU_COLOR_FORMAT;
        colorAttachments[0].begin_action = AGPU_ATTACHMENT_CLEAR;
        colorAttachments[0].end_action = AGPU_ATTACHMENT_KEEP;
        colorAttachments[0].clear_value.r = 0.0;
        colorAttachments[0].clear_value.g = 0.0;
        colorAttachments[0].clear_value.b = 0.0;
        colorAttachments[0].clear_value.a = 0;
        colorAttachments[0].sample_count = 1;

        // NormalG buffer BufferAttachment
        colorAttachments[1].format = AGPU_TEXTURE_FORMAT_R16G16_FLOAT;
        colorAttachments[1].begin_action = AGPU_ATTACHMENT_CLEAR;
        colorAttachments[1].end_action = AGPU_ATTACHMENT_KEEP;
        colorAttachments[1].clear_value.r = 0.0;
        colorAttachments[1].clear_value.g = 0.0;
        colorAttachments[1].clear_value.b = 0.0;
        colorAttachments[1].clear_value.a = 0;
        colorAttachments[1].sample_count = 1;

        // SpecularityBufferAttachment
        colorAttachments[2].format = AGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
        colorAttachments[2].begin_action = AGPU_ATTACHMENT_CLEAR;
        colorAttachments[2].end_action = AGPU_ATTACHMENT_KEEP;
        colorAttachments[2].clear_value.r = 0.0;
        colorAttachments[2].clear_value.g = 0.0;
        colorAttachments[2].clear_value.b = 0.0;
        colorAttachments[2].clear_value.a = 0;
        colorAttachments[2].sample_count = 1;

        agpu_renderpass_depth_stencil_description depthAttachment = {};
        depthAttachment.format = BEACON_AGPU_DEPTH_FORMAT;
        depthAttachment.begin_action = AGPU_ATTACHMENT_KEEP;
        depthAttachment.end_action = AGPU_ATTACHMENT_KEEP;
        depthAttachment.clear_value.depth = 0.0;
        depthAttachment.sample_count = 1;

        agpu_renderpass_description description = {};
        description.color_attachment_count = 3;
        description.color_attachments = colorAttachments;
        description.depth_stencil_attachment = &depthAttachment;

        renderer->mainDepthColorOpaqueRenderPass = agpuCreateRenderPass(device, &description);
    }

    if(!renderer->mainDepthColorRenderPass)
    {
        agpu_renderpass_color_attachment_description colorAttachment = {};
        colorAttachment.format = BEACON_AGPU_COLOR_FORMAT;
        colorAttachment.begin_action = AGPU_ATTACHMENT_KEEP;
        colorAttachment.end_action = AGPU_ATTACHMENT_KEEP;
        colorAttachment.clear_value.r = 0.0;
        colorAttachment.clear_value.g = 0.0;
        colorAttachment.clear_value.b = 0.0;
        colorAttachment.clear_value.a = 0;
        colorAttachment.sample_count = 1;

        agpu_renderpass_depth_stencil_description depthAttachment = {};
        depthAttachment.format = BEACON_AGPU_DEPTH_FORMAT;
        depthAttachment.begin_action = AGPU_ATTACHMENT_KEEP;
        depthAttachment.end_action = AGPU_ATTACHMENT_KEEP;
        depthAttachment.clear_value.depth = 0.0;
        depthAttachment.sample_count = 1;

        agpu_renderpass_description description = {};
        description.color_attachment_count = 1;
        description.color_attachments = &colorAttachment;
        description.depth_stencil_attachment = &depthAttachment;

        renderer->mainDepthColorRenderPass = agpuCreateRenderPass(device, &description);
    }

    // Create the output render pass
    if(!renderer->outputRenderPass)
    {
        agpu_renderpass_color_attachment_description colorAttachment = {};
        colorAttachment.format = BEACON_AGPU_SWAP_CHAIN_COLOR_FORMAT;
        colorAttachment.begin_action = AGPU_ATTACHMENT_CLEAR;
        colorAttachment.end_action = AGPU_ATTACHMENT_KEEP;
        colorAttachment.clear_value.r = 0.0;
        colorAttachment.clear_value.g = 0.0;
        colorAttachment.clear_value.b = 0.0;
        colorAttachment.clear_value.a = 0;
        colorAttachment.sample_count = 1;

        agpu_renderpass_description description = {};
        description.color_attachment_count = 1;
        description.color_attachments = &colorAttachment;

        renderer->outputRenderPass = agpuCreateRenderPass(device, &description);
    }

    if(!renderer->hasIntermediateBuffers || renderer->intermediateBufferWidth != displayWidth || renderer->intermediateBufferHeight != displayHeight)
    {
        agpuFinishDeviceExecution(device);

        if(renderer->hasIntermediateBuffers)
        {
            agpuReleaseTexture(renderer->mainDepthBuffer);
            agpuReleaseTexture(renderer->hdrColorBuffer);
            agpuReleaseTexture(renderer->normalGBuffer);
            agpuReleaseTexture(renderer->specularityGBuffer);
            agpuReleaseTexture(renderer->outputTexture);

            agpuReleaseFramebuffer(renderer->depthOnlyFramebuffer);
            agpuReleaseFramebuffer(renderer->hdrOpaqueFramebuffer);
            agpuReleaseFramebuffer(renderer->hdrFramebuffer);
            agpuReleaseFramebuffer(renderer->outputFramebuffer);

            renderer->mainDepthBuffer    = NULL;
            renderer->hdrColorBuffer     = NULL;
            renderer->normalGBuffer      = NULL;
            renderer->specularityGBuffer = NULL;
            renderer->outputTexture = NULL;

            renderer->depthOnlyFramebuffer = NULL;
            renderer->hdrOpaqueFramebuffer = NULL;
            renderer->hdrFramebuffer       = NULL;
            renderer->outputFramebuffer       = NULL;

            renderer->hasIntermediateBuffers = false;
        }

        renderer->intermediateBufferWidth = displayWidth;
        renderer->intermediateBufferHeight = displayHeight;

        {
            agpu_texture_description desc = {};
            desc.type = AGPU_TEXTURE_2D;
            desc.width = displayWidth;
            desc.height = displayHeight;
            desc.depth = 1;
            desc.layers = 1;
            desc.miplevels = 1;
            desc.format = BEACON_AGPU_DEPTH_FORMAT;
            desc.usage_modes = AGPU_TEXTURE_USAGE_DEPTH_ATTACHMENT | AGPU_TEXTURE_USAGE_SAMPLED;
            desc.main_usage_mode = AGPU_TEXTURE_USAGE_SAMPLED;
            desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
            desc.sample_count = 1;
            desc.sample_quality = 0;
            desc.clear_value.depth_stencil.depth = 0.0;

            renderer->mainDepthBuffer = agpuCreateTexture(device, &desc);
        }

        {
            agpu_texture_description desc = {};
            desc.type = AGPU_TEXTURE_2D;
            desc.width = displayWidth;
            desc.height = displayHeight;
            desc.depth = 1;
            desc.layers = 1;
            desc.miplevels = 1;
            desc.format = BEACON_AGPU_COLOR_FORMAT;
            desc.usage_modes = AGPU_TEXTURE_USAGE_COLOR_ATTACHMENT | AGPU_TEXTURE_USAGE_SAMPLED;
            desc.main_usage_mode = AGPU_TEXTURE_USAGE_SAMPLED;
            desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
            desc.sample_count = 1;
            desc.sample_quality = 0;

            renderer->hdrColorBuffer = agpuCreateTexture(device, &desc);
        }

        {
            agpu_texture_description desc = {};
            desc.type = AGPU_TEXTURE_2D;
            desc.width = displayWidth;
            desc.height = displayHeight;
            desc.depth = 1;
            desc.layers = 1;
            desc.miplevels = 1;
            desc.format = AGPU_TEXTURE_FORMAT_R16G16_FLOAT;
            desc.usage_modes = AGPU_TEXTURE_USAGE_COLOR_ATTACHMENT | AGPU_TEXTURE_USAGE_SAMPLED;
            desc.main_usage_mode = AGPU_TEXTURE_USAGE_SAMPLED;
            desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
            desc.sample_count = 1;
            desc.sample_quality = 0;

            renderer->normalGBuffer = agpuCreateTexture(device, &desc);
        }

        {
            agpu_texture_description desc = {};
            desc.type = AGPU_TEXTURE_2D;
            desc.width = displayWidth;
            desc.height = displayHeight;
            desc.depth = 1;
            desc.layers = 1;
            desc.miplevels = 1;
            desc.format = AGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
            desc.usage_modes = AGPU_TEXTURE_USAGE_COLOR_ATTACHMENT | AGPU_TEXTURE_USAGE_SAMPLED;
            desc.main_usage_mode = AGPU_TEXTURE_USAGE_SAMPLED;
            desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
            desc.sample_count = 1;
            desc.sample_quality = 0;

            renderer->specularityGBuffer = agpuCreateTexture(device, &desc);
        }

        {
            agpu_texture_description desc = {};
            desc.type = AGPU_TEXTURE_2D;
            desc.width = displayWidth;
            desc.height = displayHeight;
            desc.depth = 1;
            desc.layers = 1;
            desc.miplevels = 1;
            desc.format = BEACON_AGPU_SWAP_CHAIN_COLOR_FORMAT;
            desc.usage_modes = AGPU_TEXTURE_USAGE_COLOR_ATTACHMENT | AGPU_TEXTURE_USAGE_SAMPLED;
            desc.main_usage_mode = AGPU_TEXTURE_USAGE_SAMPLED;
            desc.heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL;
            desc.sample_count = 1;
            desc.sample_quality = 0;

            renderer->outputTexture = agpuCreateTexture(device, &desc);
        }
        {
            agpu_texture_view_description depthBufferViewDesc = {};
            agpuGetTextureFullViewDescription(renderer->mainDepthBuffer, &depthBufferViewDesc);
            depthBufferViewDesc.usage_mode = AGPU_TEXTURE_USAGE_DEPTH_ATTACHMENT;
            agpu_texture_view *depthBufferAttachmentView = agpuCreateTextureView(renderer->mainDepthBuffer, &depthBufferViewDesc);

            agpu_texture_view_description colorBufferViewDesc = {};
            agpuGetTextureFullViewDescription(renderer->hdrColorBuffer, &colorBufferViewDesc);
            colorBufferViewDesc.usage_mode = AGPU_TEXTURE_USAGE_COLOR_ATTACHMENT;
            agpu_texture_view *hdrColorAttachmentView = agpuCreateTextureView(renderer->hdrColorBuffer, &colorBufferViewDesc);

            agpu_texture_view_description normalBufferViewDesc = {};
            agpuGetTextureFullViewDescription(renderer->normalGBuffer, &normalBufferViewDesc);
            normalBufferViewDesc.usage_mode = AGPU_TEXTURE_USAGE_COLOR_ATTACHMENT;
            agpu_texture_view *normalGBufferAttachmentView = agpuCreateTextureView(renderer->normalGBuffer, &normalBufferViewDesc);

            agpu_texture_view_description specularityGBufferViewDesc = {};
            agpuGetTextureFullViewDescription(renderer->specularityGBuffer, &specularityGBufferViewDesc);
            specularityGBufferViewDesc.usage_mode = AGPU_TEXTURE_USAGE_COLOR_ATTACHMENT;
            agpu_texture_view *specularityGBufferAttachmentView = agpuCreateTextureView(renderer->specularityGBuffer, &specularityGBufferViewDesc);

            agpu_texture_view_description outputTextureViewDesc = {};
            agpuGetTextureFullViewDescription(renderer->outputTexture, &outputTextureViewDesc);
            outputTextureViewDesc.usage_mode = AGPU_TEXTURE_USAGE_COLOR_ATTACHMENT;
            agpu_texture_view *outputTextureAttachmentView = agpuCreateTextureView(renderer->outputTexture, &outputTextureViewDesc);

            agpu_texture_view* opaqueAttachments[] = {
                hdrColorAttachmentView,
                normalGBufferAttachmentView,
                specularityGBufferAttachmentView
            };
            renderer->hdrOpaqueFramebuffer = agpuCreateFrameBuffer(device, displayWidth, displayHeight, 3, opaqueAttachments, depthBufferAttachmentView);

            renderer->hdrFramebuffer = agpuCreateFrameBuffer(device, displayWidth, displayHeight, 1, &hdrColorAttachmentView, depthBufferAttachmentView);
            renderer->depthOnlyFramebuffer = agpuCreateFrameBuffer(device, displayWidth, displayHeight, 0, NULL, depthBufferAttachmentView);

            renderer->outputFramebuffer = agpuCreateFrameBuffer(device, displayWidth, displayHeight, 1, &outputTextureAttachmentView, NULL);
        }

        {   
            if(renderer->outputTextureIndex <= 0)
                renderer->outputTextureIndex = context->roots.agpuCommon->textureArrayBindingCount++;
            agpu_texture_view *outputTextureView = agpuGetOrCreateFullTextureView(renderer->outputTexture);
            agpuBindArrayOfSampledTextureView(context->roots.agpuCommon->texturesArrayBinding, 0, renderer->outputTextureIndex, 1, &outputTextureView);            

            if(!renderer->outputTextureHandle)
                renderer->outputTextureHandle = beacon_allocateObjectWithBehavior(context->heap, context->classes.agpuTextureHandleClass, sizeof(beacon_AGPUTextureHandle_t), BeaconObjectKindBytes);
            
            renderer->outputTextureHandle->texture = renderer->outputTexture;
            renderer->outputTextureHandle->textureView = outputTextureView;
            renderer->outputTextureHandle->textureArrayBindingIndex = renderer->outputTextureIndex;
        }

        if(!renderer->intermediateBindings)
            renderer->intermediateBindings = agpuCreateShaderResourceBinding(context->roots.agpuCommon->shaderSignature, 3);
        {
            agpu_texture_view *textureView = agpuGetOrCreateFullTextureView(renderer->hdrColorBuffer);
            agpuBindSampledTextureView(renderer->intermediateBindings, 0, textureView);
        }

        renderer->hasIntermediateBuffers = true;
    }


    return receiver;
}

static void beacon_agpuWindowRenderer_uploadPerFrameBuffer(agpu_command_list *commandList, beacon_AGPU_t *agpu, beacon_AGPUWindowRenderer_t *renderer, beacon_AGPUUpdateBuffer_t *updateBuffer)
{
    if(updateBuffer->size == 0)
        return;

    agpuCopyBuffer(commandList,
        renderer->renderingDataSubmissionBuffer, updateBuffer->offset + renderer->frameRenderingDataUploadSize*renderer->currentFrameBufferingIndex,
        agpu->gpu3DRenderingDataBuffer, updateBuffer->offset, updateBuffer->size*updateBuffer->elementSize);
}

static void beacon_agpuWindowRenderer_uploadIndicesPerFrameBuffer(agpu_command_list *commandList, beacon_AGPU_t *agpu, beacon_AGPUWindowRenderer_t *renderer, beacon_AGPUUpdateBuffer_t *updateBuffer)
{
    if(updateBuffer->size == 0)
        return;

    agpuCopyBuffer(commandList,
        renderer->renderingDataSubmissionBuffer, updateBuffer->offset + renderer->frameRenderingDataUploadSize*renderer->currentFrameBufferingIndex,
        agpu->gpu3DRenderingIndexBuffer, 0, updateBuffer->size*updateBuffer->elementSize);
}

static beacon_oop_t beacon_agpuWindowRenderer_end3DFrameRendering(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t*)receiver;
    renderer->hasPending3DRenderingCommands = true;
    return receiver;
}

static void beacon_agpuWindowRenderer_emit3DFrameRendering(beacon_context_t *context, beacon_AGPUWindowRenderer_t *renderer)
{
    if(!renderer->hasPending3DRenderingCommands)
        return;
    renderer->hasPending3DRenderingCommands = false;

    beacon_AGPUWindowRendererPerFrameState_t *thisFrameState = renderer->frameState + renderer->currentFrameBufferingIndex;
    beacon_AGPU_t *agpu = context->roots.agpuCommon;
    agpu_command_list *commandList = thisFrameState->commandList;

    int displayWidth = renderer->intermediateBufferWidth;
    int displayHeight = renderer->intermediateBufferHeight;

    // Upload the shader resources
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->renderObjectAttributes);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->renderModelAttributes);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->renderMeshPrimitiveAttributes);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->renderMaterialsAttributes);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->renderLightSourceAttributes);

    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->vertexPositions);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->vertexNormals);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->vertexTexcoords);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->vertexTangent4);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->vertexBoneIndices);
    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->vertexBoneWeights);

    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->cameraState);

    beacon_agpuWindowRenderer_uploadIndicesPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->indexData);

    // Setup the shader resources.
    agpuSetShaderSignature(thisFrameState->commandList, context->roots.agpuCommon->shaderSignature);
    agpuUseShaderResources(thisFrameState->commandList, context->roots.agpuCommon->samplerBinding);
    agpuUseShaderResources(thisFrameState->commandList, context->roots.agpuCommon->renderingDataBinding);
    agpuUseShaderResources(thisFrameState->commandList, context->roots.agpuCommon->texturesArrayBinding);
    agpuUseShaderResources(thisFrameState->commandList, renderer->intermediateBindings);

    agpuUseIndexBuffer(commandList, context->roots.agpuCommon->gpu3DRenderingIndexBuffer);
    agpuUseDrawIndirectBuffer(commandList, context->roots.agpuCommon->renderDrawIndirectBuffer);

    // Perform the main geometry culling.
    agpuUseComputeShaderResources(thisFrameState->commandList, context->roots.agpuCommon->samplerBinding);
    agpuUseComputeShaderResources(thisFrameState->commandList, context->roots.agpuCommon->renderingDataBinding);
    agpuUseComputeShaderResources(thisFrameState->commandList, context->roots.agpuCommon->texturesArrayBinding);
    agpuUseComputeShaderResources(thisFrameState->commandList, renderer->intermediateBindings);

    // Push the constants.
    {
        uint32_t renderObjectsSize = agpu->renderObjectAttributes.size;
        uint32_t lightSourceCount = agpu->renderLightSourceAttributes.size;
        uint32_t nullPushConstant = 0;
    
        agpuPushConstants(commandList, 0, 4, &renderObjectsSize);
        agpuPushConstants(commandList, 4, 4, &lightSourceCount);
        agpuPushConstants(commandList, 8, 4, &nullPushConstant);
        agpuPushConstants(commandList, 12, 4, &nullPushConstant);
    
        int hasTopLeftNDCOriginValue = agpuHasTopLeftNdcOrigin(context->roots.agpuCommon->device);
        agpuPushConstants(thisFrameState->commandList, 16, 4, &hasTopLeftNDCOriginValue);

        float framebufferReciprocalExtentX = 1.0f / displayWidth;
        float framebufferReciprocalExtentY = 1.0f / displayHeight;
        agpuPushConstants(thisFrameState->commandList, 24, 4, &framebufferReciprocalExtentX);
        agpuPushConstants(thisFrameState->commandList, 28, 4, &framebufferReciprocalExtentY);
    }

    agpuUsePipelineState(commandList, agpu->clearRenderChunkData);
    agpuDispatchCompute(commandList, 1, 1, 1);
    agpuMemoryBarrier(commandList, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_ACCESS_SHADER_WRITE, AGPU_ACCESS_SHADER_READ);

    agpuUsePipelineState(commandList, agpu->cullOpaqueObjects);
    agpuDispatchCompute(commandList, (agpu->renderObjectAttributes.size + 127) / 128, 1, 1);
    agpuMemoryBarrier(commandList, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_ACCESS_SHADER_WRITE, AGPU_ACCESS_SHADER_READ);

    agpuUsePipelineState(commandList, agpu->makeDrawIndirectPipeline);
    agpuDispatchCompute(commandList, (agpu->renderObjectAttributes.size + 127) / 128, 1, 1);
    agpuMemoryBarrier(commandList, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_PIPELINE_STAGE_VERTEX_SHADER | AGPU_PIPELINE_STAGE_FRAGMENT_SHADER | AGPU_PIPELINE_STAGE_DRAW_INDIRECT, AGPU_ACCESS_SHADER_WRITE, AGPU_ACCESS_SHADER_READ);
    
    // Compute the lighting grid.
    {
        // Transform light sources to view space.
        agpuUsePipelineState(commandList, agpu->transformLightsToViewPipeline);
        agpuDispatchCompute(commandList, (agpu->renderLightSourceAttributes.size + 127) / 128, 1, 1);
        agpuMemoryBarrier(commandList, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, (AGPU_PIPELINE_STAGE_COMPUTE_SHADER | AGPU_PIPELINE_STAGE_FRAGMENT_SHADER), AGPU_ACCESS_SHADER_WRITE, AGPU_ACCESS_SHADER_READ);

        // See: recordLightGridComputationCommands.
        uint32_t workgroupCountX = (BEACON_AGPU_LIGHT_GRID_WIDTH + 3) / 4;
        uint32_t workgroupCountY = (BEACON_AGPU_LIGHT_GRID_HEIGHT + 3) / 4;
        uint32_t workgroupCountZ = (BEACON_AGPU_LIGHT_GRID_DEPTH + 3) / 4;
        
        uint32_t workgroupCount = (BEACON_AGPU_LIGHT_GRID_CELL_COUNT + 63) / 64;

        agpuUsePipelineState(commandList, agpu->lightGridComputationPipeline);
        agpuDispatchCompute(commandList, workgroupCountX, workgroupCountY, workgroupCountZ);
        agpuMemoryBarrier(commandList, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_ACCESS_SHADER_WRITE, AGPU_ACCESS_SHADER_READ);

        agpuUsePipelineState(commandList, agpu->lightClusterBeginComputationPipeline);
        agpuDispatchCompute(commandList, 1, 1, 1);
        agpuMemoryBarrier(commandList, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_ACCESS_SHADER_WRITE, AGPU_ACCESS_SHADER_READ);

        agpuUsePipelineState(commandList, agpu->lightClusterListComputationPipeline);
        agpuDispatchCompute(commandList, workgroupCount, 1, 1);
        agpuMemoryBarrier(commandList, AGPU_PIPELINE_STAGE_COMPUTE_SHADER, AGPU_PIPELINE_STAGE_COMPUTE_SHADER | AGPU_PIPELINE_STAGE_FRAGMENT_SHADER, AGPU_ACCESS_SHADER_WRITE, AGPU_ACCESS_SHADER_READ);
    }

    // Shadow maps
    {
        agpuBeginRenderPass(commandList, renderer->shadowMapAtlasRenderPass, agpu->shadowMapFramebuffer, false);
        for(size_t i = 0; i < agpu->numberOfShadowCastingLightSources; ++i)
        {
            beacon_AGPUShadowCastingLight_t *lightSource = agpu->shadowCastingLightSources + i;
            for(uint32_t partIndex = 0; partIndex < lightSource->shadowMapPartCount; ++partIndex)
            {
                beacon_AGPUShadowMapAtlasAllocation_t *allocation = lightSource->atlasAllocations + partIndex;
                beacon_RenderVector2_t offset = allocation->offset;
                beacon_RenderVector2_t extent = allocation->shadowMapExtent;
                agpuSetViewport(commandList, offset.x, offset.y, extent.x, extent.y);
                agpuSetScissor(commandList, offset.x, offset.y, extent.x, extent.y);

                uint32_t shadowMapLightSourceIndex = lightSource->renderLightSourceIndex;
                uint32_t shadowMapComponent = partIndex;
                agpuPushConstants(commandList, 8, 4, &shadowMapLightSourceIndex);
                agpuPushConstants(commandList, 12, 4, &shadowMapComponent);

                agpuUsePipelineState(commandList, agpu->clearDepthPipeline);
                agpuDrawArrays(commandList, 3, 1, 0, 0);

                agpuUsePipelineState(commandList, agpu->shadowMapDepthPipeline);
                agpuDrawElementsIndirect(commandList, 0, agpu->renderObjectAttributes.size);
            }
        }
        agpuEndRenderPass(commandList);
    }

    // Depth only render pass
    agpuBeginRenderPass(commandList, renderer->mainDepthRenderPass, renderer->depthOnlyFramebuffer, false);
    agpuSetViewport(commandList, 0, 0, displayWidth, displayHeight);
    agpuSetScissor(commandList, 0, 0, displayWidth, displayHeight);
    
    agpuUsePipelineState(commandList, agpu->opaqueDepthOnlyPipeline);
    agpuDrawElementsIndirect(commandList, 0, agpu->renderObjectAttributes.size);

    agpuEndRenderPass(commandList);

    // Opaque color
    agpuBeginRenderPass(commandList, renderer->mainDepthColorOpaqueRenderPass, renderer->hdrOpaqueFramebuffer, false);
    agpuSetViewport(commandList, 0, 0, displayWidth, displayHeight);
    agpuSetScissor(commandList, 0, 0, displayWidth, displayHeight);

    // Draw the sky and the background.
    agpuUsePipelineState(commandList, agpu->daySkyPipeline);
    agpuDrawArrays(commandList, 3, 1, 0, 0);

    // Draw the opaque elements.
    agpuUsePipelineState(commandList, agpu->opaqueColorPipeline);
    agpuDrawElementsIndirect(commandList, 0, agpu->renderObjectAttributes.size);

    agpuEndRenderPass(commandList);

    // Non-opaque
    agpuBeginRenderPass(commandList, renderer->mainDepthColorRenderPass, renderer->hdrFramebuffer, false);
    agpuSetViewport(commandList, 0, 0, displayWidth, displayHeight);
    agpuSetScissor(commandList, 0, 0, displayWidth, displayHeight);

    // TODO: Render the translucent objects with color.
    agpuEndRenderPass(commandList);

    // Output frame
    agpuBeginRenderPass(commandList, renderer->outputRenderPass, renderer->outputFramebuffer, false);
    agpuSetViewport(commandList, 0, 0, displayWidth, displayHeight);
    agpuSetScissor(commandList, 0, 0, displayWidth, displayHeight);

    agpuUsePipelineState(commandList, agpu->toneMappingPipeline);
    agpuDrawArrays(commandList, 3, 1, 0, 0);

    agpuEndRenderPass(commandList);
}

static beacon_oop_t beacon_agpuWindowRenderer_get3DOutputTextureHandle(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t *)receiver;
    return (beacon_oop_t)renderer->outputTextureHandle;
}

static beacon_oop_t beacon_agpuWindowRenderer_endFrame(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t*)receiver;
    beacon_AGPUWindowRendererPerFrameState_t *thisFrameState = renderer->frameState + renderer->currentFrameBufferingIndex;
    beacon_AGPU_t *agpu = context->roots.agpuCommon;

    beacon_agpuWindowRenderer_uploadPerFrameBuffer(thisFrameState->commandList, agpu, renderer, &agpu->guiData);

    beacon_agpuWindowRenderer_emit3DFrameRendering(context, renderer);

    agpuSetShaderSignature(thisFrameState->commandList, context->roots.agpuCommon->shaderSignature);
    agpuUseShaderResources(thisFrameState->commandList, context->roots.agpuCommon->samplerBinding);
    agpuUseShaderResources(thisFrameState->commandList, context->roots.agpuCommon->renderingDataBinding);
    agpuUseShaderResources(thisFrameState->commandList, context->roots.agpuCommon->texturesArrayBinding);

    // Begin the renderpass to the main buffer.
    agpu_framebuffer *backbuffer = agpuGetCurrentBackBuffer(renderer->swapChain->swapChain);
    agpuBeginRenderPass(thisFrameState->commandList, context->roots.agpuCommon->mainRenderPass, backbuffer, false);

    int screenWidth = agpuGetSwapChainWidth(renderer->swapChain->swapChain);
    int screenHeight = agpuGetSwapChainHeight(renderer->swapChain->swapChain);
    agpuSetViewport(thisFrameState->commandList, 0, 0, screenWidth, screenHeight);
    agpuSetScissor(thisFrameState->commandList, 0, 0, screenWidth, screenHeight);

    // Push the constants.
    {
        int hasTopLeftNDCOriginValue = agpuHasTopLeftNdcOrigin(context->roots.agpuCommon->device);
        agpuPushConstants(thisFrameState->commandList, 16, 4, &hasTopLeftNDCOriginValue);

        float framebufferReciprocalExtentX = 1.0f / screenWidth;
        float framebufferReciprocalExtentY = 1.0f / screenHeight;
        agpuPushConstants(thisFrameState->commandList, 24, 4, &framebufferReciprocalExtentX);
        agpuPushConstants(thisFrameState->commandList, 28, 4, &framebufferReciprocalExtentY);
    }

    agpuUsePipelineState(thisFrameState->commandList, context->roots.agpuCommon->guiPipelineState);
    agpuDrawArrays(thisFrameState->commandList, 4, context->roots.agpuCommon->guiData.size, 0, 0);

    // End the main buffer renderpass.
    agpuEndRenderPass(thisFrameState->commandList);

    agpuCloseCommandList(thisFrameState->commandList);
    agpuAddCommandListsAndSignalFence(renderer->commandQueue, 1, &thisFrameState->commandList, thisFrameState->fence);
    thisFrameState->hasSubmittedToQueue = true;
    agpuSwapBuffers(renderer->swapChain->swapChain);

    agpuReleaseFramebuffer(backbuffer);
    
    renderer->currentFrameBufferingIndex = (renderer->currentFrameBufferingIndex + 1) % BEACON_AGPU_FRAMEBUFFERING_COUNT;
    return receiver;
}

static void beacon_agpuWindowRenderer_renderText(beacon_context_t *context, beacon_AGPUWindowRenderer_t *renderer, beacon_FormTextRenderingElement_t *renderingElement)
{
    beacon_AGPUWindowRendererPerFrameState_t *thisFrameState = renderer->frameState + renderer->currentFrameBufferingIndex;
    if(!renderingElement->fontFace || !renderingElement->fontFace->atlasForm)
        return;
    
    beacon_FontFace_t *fontFace = renderingElement->fontFace;
    beacon_AGPUTextureHandle_t *handle = beacon_getValidTextureHandleForFontFaceForm(context, renderingElement->fontFace->atlasForm);

    int formWidth = beacon_decodeSmallInteger(renderingElement->fontFace->atlasForm->width);
    int formHeight = beacon_decodeSmallInteger(renderingElement->fontFace->atlasForm->height);

    float rectMinX = beacon_decodeSmallNumber(renderingElement->super.rectangle->origin->x);
    float rectMinY = beacon_decodeSmallNumber(renderingElement->super.rectangle->origin->y);
    float rectMaxX = beacon_decodeSmallNumber(renderingElement->super.rectangle->corner->x);
    float rectMaxY = beacon_decodeSmallNumber(renderingElement->super.rectangle->corner->y);

    float ascent = beacon_decodeSmallNumber(fontFace->ascent);

    float baselineX = rectMinX;
    float baselineY = rectMinY + ascent;
    size_t stringSize = renderingElement->text->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < stringSize; ++i)
    {
        char c = renderingElement->text->data[i];
        if(c < ' ')
            continue;

        stbtt_aligned_quad quadToDraw = {};
        stbtt_GetBakedQuad((stbtt_bakedchar*)fontFace->charData->elements, formWidth, formHeight, c - 31, &baselineX, &baselineY, &quadToDraw, true);

        beacon_GuiRenderingElement_t quad = {
            .type = BeaconGuiTextCharacter,
            .texture = handle->textureArrayBindingIndex,
            .borderSize = beacon_decodeSmallNumber(renderingElement->super.borderSize),
            .borderRoundRadius = beacon_decodeSmallNumber(renderingElement->super.borderRoundRadius),

            .rectangleMinX = quadToDraw.x0,
            .rectangleMinY = quadToDraw.y0,
            .rectangleMaxX = quadToDraw.x1,
            .rectangleMaxY = quadToDraw.y1,

            .imageRectangleMinX = quadToDraw.s0,
            .imageRectangleMinY = quadToDraw.t0,
            .imageRectangleMaxX = quadToDraw.s1,
            .imageRectangleMaxY = quadToDraw.t1,

            .firstColor = {
                beacon_decodeSmallNumber(renderingElement->color->r),
                beacon_decodeSmallNumber(renderingElement->color->g),
                beacon_decodeSmallNumber(renderingElement->color->b),
                beacon_decodeSmallNumber(renderingElement->color->a),
            },
        };

        ((beacon_GuiRenderingElement_t*)context->roots.agpuCommon->guiData.thisFrameBuffer)[context->roots.agpuCommon->guiData.size++] = quad;
    }
}

static beacon_oop_t beacon_agpuWindowRenderer_renderQuadList(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t*)receiver;
    beacon_AGPUWindowRendererPerFrameState_t *thisFrameState = renderer->frameState + renderer->currentFrameBufferingIndex;

    beacon_Array_t *quadList = (beacon_Array_t*)arguments[0];
    size_t quadCount = quadList->super.super.super.super.super.header.slotCount;
    for(size_t i = 0; i < quadCount; ++i)
    {
        beacon_oop_t quadOop = quadList->elements[i];
        beacon_Behavior_t *quadClass = beacon_getClass(context, quadOop);
        if(quadClass == context->classes.formSolidRectangleRenderingElementClass)
        {
            beacon_FormSolidRectangleRenderingElement_t *solidElement = (beacon_FormSolidRectangleRenderingElement_t*)quadOop;
            beacon_GuiRenderingElement_t quad = {
                .type = BeaconGuiSolidRectangle,
                .texture = -1,
                .borderSize = beacon_decodeSmallNumber(solidElement->super.borderSize),
                .borderRoundRadius = beacon_decodeSmallNumber(solidElement->super.borderRoundRadius),

                .rectangleMinX = beacon_decodeSmallNumber(solidElement->super.rectangle->origin->x),
                .rectangleMinY = beacon_decodeSmallNumber(solidElement->super.rectangle->origin->y),
                .rectangleMaxX = beacon_decodeSmallNumber(solidElement->super.rectangle->corner->x),
                .rectangleMaxY = beacon_decodeSmallNumber(solidElement->super.rectangle->corner->y),

                .firstColor = {
                    beacon_decodeSmallNumber(solidElement->color->r),
                    beacon_decodeSmallNumber(solidElement->color->g),
                    beacon_decodeSmallNumber(solidElement->color->b),
                    beacon_decodeSmallNumber(solidElement->color->a),
                },

                .borderColor = {
                    beacon_decodeSmallNumber(solidElement->borderColor->r),
                    beacon_decodeSmallNumber(solidElement->borderColor->g),
                    beacon_decodeSmallNumber(solidElement->borderColor->b),
                    beacon_decodeSmallNumber(solidElement->borderColor->a),
                },
            };

            ((beacon_GuiRenderingElement_t*)context->roots.agpuCommon->guiData.thisFrameBuffer)[context->roots.agpuCommon->guiData.size++] = quad;

        }
        else if(quadClass == context->classes.formTextRenderingElementClass)
        {
            beacon_FormTextRenderingElement_t *textElement = (beacon_FormTextRenderingElement_t*)quadOop;
            beacon_agpuWindowRenderer_renderText(context, renderer, textElement);
        }
        else if(quadClass == context->classes.formTextureHandleRenderingElementClass)
        {
            beacon_FormTextureHandleRenderingElement_t *textureElement = (beacon_FormTextureHandleRenderingElement_t*)quadOop;
            if(!textureElement->textureHandle)
                continue;

            beacon_AGPUTextureHandle_t *textureHandle = (beacon_AGPUTextureHandle_t *)textureElement->textureHandle;

            beacon_GuiRenderingElement_t quad = {
                .type = BeaconGuiTexturedRectangle,
                .texture = textureHandle->textureArrayBindingIndex,
                .borderSize = beacon_decodeSmallNumber(textureElement->super.borderSize),
                .borderRoundRadius = beacon_decodeSmallNumber(textureElement->super.borderRoundRadius),

                .rectangleMinX = beacon_decodeSmallNumber(textureElement->super.rectangle->origin->x),
                .rectangleMinY = beacon_decodeSmallNumber(textureElement->super.rectangle->origin->y),
                .rectangleMaxX = beacon_decodeSmallNumber(textureElement->super.rectangle->corner->x),
                .rectangleMaxY = beacon_decodeSmallNumber(textureElement->super.rectangle->corner->y),

                .imageRectangleMinX = 0,
                .imageRectangleMinY = 0,
                .imageRectangleMaxX = 1,
                .imageRectangleMaxY = 1,    
            };

            ((beacon_GuiRenderingElement_t*)context->roots.agpuCommon->guiData.thisFrameBuffer)[context->roots.agpuCommon->guiData.size++] = quad;
        }
    }

    return receiver;
}

static beacon_oop_t beacon_AGPU_platform(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPU_t *agpu = (beacon_AGPU_t*)receiver;
    if(!agpu->platform)
        beacon_agpu_getPlatform(context, agpu);
    return beacon_boxExternalAddress(context, agpu->platform);
}

static beacon_oop_t beacon_AGPU_device(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPU_t *agpu = (beacon_AGPU_t*)receiver;
    if(!agpu->device)
        beacon_agpu_getDevice(context, agpu);
    return beacon_boxExternalAddress(context, agpu->device);
}

static beacon_oop_t beacon_AGPU_uniqueInstance(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    return (beacon_oop_t)context->roots.agpuCommon;
}

static size_t beacon_agpu_pushVertexPosition(beacon_AGPU_t *agpu, float x, float y, float z)
{
    size_t index = agpu->vertexPositions.size;
    beacon_RenderPackedVector3_t position = {x, y, z};
    beacon_RenderPackedVector3_t *positionBuffer = agpu->vertexPositions.thisFrameBuffer;
    positionBuffer[agpu->vertexPositions.size++] = position;
    return index;
}

static size_t beacon_agpu_pushVertexNormal(beacon_AGPU_t *agpu, float x, float y, float z)
{
    size_t index = agpu->vertexNormals.size;
    beacon_RenderPackedVector3_t normal = {x, y, z};
    beacon_RenderPackedVector3_t *normalBuffer = agpu->vertexNormals.thisFrameBuffer;
    normalBuffer[agpu->vertexNormals.size++] = normal;
    return index;
}

static size_t beacon_agpu_pushTangents4(beacon_AGPU_t *agpu, float x, float y, float z, float w)
{
    size_t index = agpu->vertexTangent4.size;
    beacon_RenderVector4_t tangent = {x, y, z, w};
    beacon_RenderVector4_t *tangents4Buffer = agpu->vertexTangent4.thisFrameBuffer;
    tangents4Buffer[agpu->vertexTangent4.size++] = tangent;
    return index;
}

static size_t beacon_agpu_pushVertexTexcoord(beacon_AGPU_t *agpu, float s, float t)
{
    size_t index = agpu->vertexTexcoords.size;
    beacon_RenderVector2_t texcoord = {s, t};
    beacon_RenderVector2_t *texcoordBuffer = agpu->vertexTexcoords.thisFrameBuffer;
    texcoordBuffer[agpu->vertexTexcoords.size++] = texcoord;
    return index;
}

static size_t beacon_agpu_pushIndex(beacon_AGPU_t *agpu, uint32_t indexValue)
{
    size_t oldIndex = agpu->indexData.size;
    uint32_t *indexBuffer = agpu->indexData.thisFrameBuffer;
    indexBuffer[agpu->indexData.size++] = indexValue;
    return oldIndex;
}

static size_t beacon_agpu_pushTriangle(beacon_AGPU_t *agpu, uint32_t firstIndex, uint32_t secondIndex, uint32_t thirdIndex)
{
    size_t oldIndex = beacon_agpu_pushIndex(agpu, firstIndex);
    beacon_agpu_pushIndex(agpu, secondIndex);
    beacon_agpu_pushIndex(agpu, thirdIndex);
    return oldIndex;
}

static size_t beacon_agpu_pushMeshPrimitiveAttributes(beacon_AGPU_t *agpu, beacon_RenderMeshPrimitiveAttributes_t meshPrimitive)
{
    size_t index = agpu->renderMeshPrimitiveAttributes.size;
    beacon_RenderMeshPrimitiveAttributes_t *buffer = agpu->renderMeshPrimitiveAttributes.thisFrameBuffer;
    buffer[agpu->renderMeshPrimitiveAttributes.size++] = meshPrimitive;
    return index;
}

static size_t beacon_agpu_pushModelAttributes(beacon_AGPU_t *agpu, beacon_RenderModelAttributes_t modelAttributes)
{
    size_t index = agpu->renderModelAttributes.size;
    beacon_RenderModelAttributes_t *buffer = agpu->renderModelAttributes.thisFrameBuffer;
    buffer[agpu->renderModelAttributes.size++] = modelAttributes;
    return index;
}

static size_t beacon_agpu_pushRenderObjectAttributes(beacon_AGPU_t *agpu, beacon_RenderObjectAttributes_t objectAttributes)
{
    size_t index = agpu->renderObjectAttributes.size;
    beacon_RenderObjectAttributes_t *buffer = agpu->renderObjectAttributes.thisFrameBuffer;
    buffer[agpu->renderObjectAttributes.size++] = objectAttributes;
    return index;
}

static size_t beacon_agpu_pushRenderCameraState(beacon_AGPU_t *agpu, beacon_RenderCameraState_t cameraState)
{
    size_t index = agpu->cameraState.size;
    beacon_RenderCameraState_t *buffer = agpu->cameraState.thisFrameBuffer;
    buffer[agpu->cameraState.size++] = cameraState;
    return index;
}

static size_t beacon_agpu_pushRenderLightSource(beacon_AGPU_t *agpu, beacon_RenderLightSource_t lightSourceAttributes)
{
    size_t index = agpu->renderLightSourceAttributes.size;
    beacon_RenderLightSource_t *buffer = agpu->renderLightSourceAttributes.thisFrameBuffer;
    buffer[agpu->renderLightSourceAttributes.size++] = lightSourceAttributes;
    return index;
}

static double beacon_agpu_computeLightGridDepthSliceScale(double lightGridDepth, double nearDistance, double farDistance)
{
    return lightGridDepth / log(farDistance / nearDistance);
}

static double beacon_agpu_computeLightGridDepthSliceOffset(double lightGridDepth, double nearDistance, double farDistance)
{
    return -lightGridDepth * log(nearDistance) / log(farDistance / nearDistance);
}

static beacon_RenderVector2_t beacon_agpu_computeLightGridDepthSliceScaleOffset(double lightGridDepth, double nearDistance, double farDistance)
{
    double scale = beacon_agpu_computeLightGridDepthSliceScale(lightGridDepth, nearDistance, farDistance);
    double offset = beacon_agpu_computeLightGridDepthSliceOffset(lightGridDepth, nearDistance, farDistance);
    
    //Formula from: http://www.aortiz.me/2018/12/21/CG.html#building-a-cluster-grid [January 2025]
    beacon_RenderVector2_t vector = {scale, offset};
    return vector;
}

static beacon_oop_t beacon_agpuWindowRenderer_addSceneCamera(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.sceneCameraClass);

    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t *)receiver;
    beacon_SceneCamera_t *sceneCamera = (beacon_SceneCamera_t*)arguments[0];

    beacon_AGPU_t *agpu = context->roots.agpuCommon;
    agpu_device *device = agpu->device;

    bool flipVertically = agpuHasTopLeftNdcOrigin(device);
    float nearDistance = beacon_decodeNumberAsDouble(context, sceneCamera->nearDistance);
    float farDistance = beacon_decodeNumberAsDouble(context, sceneCamera->farDistance);
    float fovY = beacon_decodeNumberAsDouble(context, sceneCamera->fovY);
    float focalDistance = beacon_decodeNumberAsDouble(context, sceneCamera->focalDistance);
    float aspectRatio = (float)renderer->intermediateBufferWidth / renderer->intermediateBufferHeight;
    bool isPerspective = sceneCamera->isPerspective == context->roots.trueValue;

    beacon_Vector3_t *translationArgument = (beacon_Vector3_t*)sceneCamera->location;
    beacon_Matrix3x3_t *orientation = (beacon_Matrix3x3_t*)sceneCamera->orientation;

    beacon_RenderVector3_t translation = {translationArgument->x, translationArgument->y, translationArgument->z};
    beacon_RenderMatrix3x3_t renderOrientationMatrix = {
        .m11 = orientation->m11, .m12 = orientation->m12, .m13 = orientation->m13,
        .m21 = orientation->m21, .m22 = orientation->m22, .m23 = orientation->m23,
        .m31 = orientation->m31, .m32 = orientation->m32, .m33 = orientation->m33,
    };

    beacon_RenderMatrix4x4_t inverseViewMatrix = beacon_RenderMatrix4x4_withMatrix3x3AndTranslation(renderOrientationMatrix, translation);
    beacon_RenderMatrix4x4_t viewMatrix = beacon_RenderMatrix4x4_inverse(inverseViewMatrix);

    beacon_RenderMatrix4x4_t projection;
    if(isPerspective)
    {
        projection = beacon_RenderMatrix4x4_reverseDepthPerspective(fovY, aspectRatio, nearDistance, farDistance, flipVertically);
    }
    else
	{
        float hh = tan((fovY / 2 ) * (M_PI / 180.0)) * focalDistance;
        float hw = hh * aspectRatio;
        if(flipVertically)
            projection = beacon_RenderMatrix4x4_reverseDepthOrtho(-hw, hw, hh, -hh, nearDistance, farDistance);
        else
            projection = beacon_RenderMatrix4x4_reverseDepthOrtho(-hw, hw, -hh, hh, nearDistance, farDistance);
    }
    
    beacon_RenderMatrix4x4_t inverseProjection = beacon_RenderMatrix4x4_inverse(projection);

    beacon_RenderCameraState_t cameraRenderState = {
        .framebufferExtent = {renderer->intermediateBufferWidth, renderer->intermediateBufferHeight},
        .framebufferReciprocalExtent = {1.0/renderer->intermediateBufferWidth, 1.0/renderer->intermediateBufferHeight},

        .flipVertically = flipVertically,
        .nearDistance = nearDistance,
        .farDistance = farDistance,
    
        .timeOfSimulation = beacon_decodeNumberAsDouble(context, sceneCamera->timeOfSimulation),
        .timeOfDay = beacon_decodeNumberAsDouble(context, sceneCamera->timeOfDay),
        .exposure = beacon_decodeNumberAsDouble(context, sceneCamera->exposure),
    
        .ambientLightSource = {
            sceneCamera->ambientLightSource->x,
            sceneCamera->ambientLightSource->y,
            sceneCamera->ambientLightSource->z
        },
            
        .hasTopLeftNDCOrigin = agpuHasTopLeftNdcOrigin(device),
        .hasBottomLeftTextureCoordinates = agpuHasBottomLeftTextureCoordinates(device),

        .lightGridExtentX = BEACON_AGPU_LIGHT_GRID_WIDTH,
        .lightGridExtentY = BEACON_AGPU_LIGHT_GRID_HEIGHT,
        .lightGridExtentZ = BEACON_AGPU_LIGHT_GRID_DEPTH,
        .lightGridDepthSliceScaleOffset = beacon_agpu_computeLightGridDepthSliceScaleOffset(BEACON_AGPU_LIGHT_GRID_DEPTH, nearDistance, farDistance),

        .projectionMatrix = projection,
        .inverseProjectionMatrix = inverseProjection,

        .viewMatrix = viewMatrix,
        .inverseViewMatrix = inverseViewMatrix,
    };
    beacon_Frustum_t viewFrustum = {};
    beacon_Frustum_setPerspective(&viewFrustum, fovY, aspectRatio, nearDistance, farDistance);
    
    beacon_Frustum_t worldFrustum = {};
    beacon_Frustum_transformWithMatrix4x4(&worldFrustum, &viewFrustum, inverseViewMatrix);
    
    cameraRenderState.worldFrustum = worldFrustum;

    beacon_agpu_pushRenderCameraState(agpu, cameraRenderState);
    return receiver;
}

static uint32_t beacon_agpuWindowRenderer_uploadPrimitive(beacon_context_t *context, beacon_AGPUWindowRenderer_t *renderer, beacon_MeshPrimitive_t *primitive)
{
    if(primitive->super.lastUploadedFrame == beacon_encodeSmallInteger(renderer->renderingFrameIndex))
        return beacon_decodeSmallInteger(primitive->super.uploadedIndex);

    beacon_AGPU_t *agpu = context->roots.agpuCommon;

    size_t positionsCount = primitive->positions->super.super.super.super.super.header.slotCount;
    size_t normalsCount = primitive->normals->super.super.super.super.super.header.slotCount;
    size_t tangents4Count = primitive->tangents4->super.super.super.super.super.header.slotCount;
    size_t texcoordsCount = primitive->texcoords->super.super.super.super.super.header.slotCount;
    size_t indexCount = primitive->indices->super.super.super.super.super.header.slotCount;

    size_t positionBufferIndex = agpu->vertexPositions.size;
    size_t normalBufferIndex = agpu->vertexNormals.size;
    size_t tangents4Index = agpu->vertexTangent4.size;
    size_t texcoordsIndex = agpu->vertexTexcoords.size;
    size_t indexBufferIndex = agpu->indexData.size;

    for(size_t i = 0; i < positionsCount; ++i)
    {
        beacon_Vector3_t *v = (beacon_Vector3_t*)primitive->positions->elements[i];
        beacon_agpu_pushVertexPosition(agpu, v->x, v->y, v->z);
    }

    for(size_t i = 0; i < normalsCount; ++i)
    {
        beacon_Vector3_t *v = (beacon_Vector3_t*)primitive->normals->elements[i];
        beacon_agpu_pushVertexNormal(agpu, v->x, v->y, v->z);
    }

    for(size_t i = 0; i < indexCount; ++i)
    {
        intptr_t index = beacon_decodeSmallInteger(primitive->indices->elements[i]);
        beacon_agpu_pushIndex(agpu, index);
    }

    beacon_RenderMeshPrimitiveAttributes_t meshPrimitive = {
        .materialIndex = -1,
        .vertexCount = positionsCount,
        .firstPositionIndex = positionsCount > 0 ? positionBufferIndex : -1,
        .firstNormalIndex = normalsCount > 0 ? normalBufferIndex : -1,
        .firstTangents4Index = -1,
        .firstTexcoordIndex = -1,

        .indexCount = indexCount,
        .firstIndexPosition = indexBufferIndex,
    };
    size_t meshPrimitiveIndex = beacon_agpu_pushMeshPrimitiveAttributes(agpu, meshPrimitive);
    primitive->super.uploadedIndex = beacon_encodeSmallInteger(meshPrimitiveIndex);
    primitive->super.lastUploadedFrame = beacon_encodeSmallInteger(renderer->renderingFrameIndex);
    return meshPrimitiveIndex;
}

static uint32_t beacon_agpuWindowRenderer_uploadModel(beacon_context_t *context, beacon_AGPUWindowRenderer_t *renderer, beacon_Model3D_t *model)
{
    if(model->super.lastUploadedFrame == beacon_encodeSmallInteger(renderer->renderingFrameIndex))
        return beacon_decodeSmallInteger(model->super.uploadedIndex);

    beacon_AGPU_t *agpu = context->roots.agpuCommon;
    uint32_t primitiveCount = model->primitives->super.super.super.super.super.header.slotCount;
    uint32_t firstPrimitiveIndex = 0;
    for(uint32_t i = 0; i < primitiveCount; ++i)
    {
        beacon_MeshPrimitive_t *primitive = (beacon_MeshPrimitive_t *)model->primitives->elements[i];
        uint32_t primitiveIndex = beacon_agpuWindowRenderer_uploadPrimitive(context, renderer, primitive);
        if(i == 0)
            firstPrimitiveIndex = primitiveIndex;
    }

    beacon_RenderModelAttributes_t modelAttributes = {
        .submeshCount = primitiveCount,
        .firstSubmeshIndex = firstPrimitiveIndex
    };

    size_t modelIndex = beacon_agpu_pushModelAttributes(agpu, modelAttributes);
    model->super.uploadedIndex = beacon_encodeSmallInteger(modelIndex);
    model->super.lastUploadedFrame = beacon_encodeSmallInteger(renderer->renderingFrameIndex);
    return modelIndex;
}

static beacon_oop_t beacon_agpuWindowRenderer_addRenderObject(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.renderObject3DClass);

    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t *)receiver;
    beacon_AGPU_t *agpu = context->roots.agpuCommon;

    beacon_RenderObject3D_t *renderObject3D = (beacon_RenderObject3D_t*)arguments[0];
    beacon_RenderMatrix4x4_t modelMatrix = beacon_RenderMatrix4x4_fromMatrix4x4(renderObject3D->modelMatrix);
    beacon_RenderMatrix4x4_t inverseModelMatrix = beacon_RenderMatrix4x4_inverse(modelMatrix);

    beacon_RenderObjectAttributes_t objectAttributes = {
        .modelMatrix = modelMatrix,
        .inverseModelMatrix = inverseModelMatrix,
        .modelIndex = beacon_agpuWindowRenderer_uploadModel(context, renderer, renderObject3D->model),
        .isSelected = renderObject3D->isSelected == context->roots.trueValue,
        .selectionColor = {
            .x = beacon_decodeSmallNumber(renderObject3D->selectionColor->r),
            .y = beacon_decodeSmallNumber(renderObject3D->selectionColor->g),
            .z = beacon_decodeSmallNumber(renderObject3D->selectionColor->b),
            .w = beacon_decodeSmallNumber(renderObject3D->selectionColor->a),
        }
    };

    beacon_agpu_pushRenderObjectAttributes(agpu, objectAttributes);

    return receiver;
}



static beacon_oop_t beacon_agpuWindowRenderer_addLightSource(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, argumentCount == 1);
    BeaconAssert(context, beacon_getClass(context, arguments[0]) == context->classes.lightSourceClass);

    beacon_LightSource_t *lightSource = (beacon_LightSource_t *)arguments[0];
    
    beacon_Vector3_t *intensity = lightSource->intensity;
    double influenceRadius = beacon_decodeNumberAsDouble(context, lightSource->influenceRadius);

    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t *)receiver;
    beacon_AGPU_t *agpu = context->roots.agpuCommon;

    bool flipProjectionVertically = agpuHasTopLeftNdcOrigin(agpu->device);
    bool flipTextureVertically = agpuHasTopLeftNdcOrigin(agpu->device) == agpuHasBottomLeftTextureCoordinates(agpu->device);

    double innerSpotCutoff = beacon_decodeNumberAsDouble(context, lightSource->innerSpotCutoff);
    double outerSpotCutoff = beacon_decodeNumberAsDouble(context, lightSource->outerSpotCutoff);

    beacon_RenderMatrix3x3_t orientationMat;
    if(beacon_getClass(context, lightSource->orientation) == context->classes.quaternionClass)
    {
        beacon_Quaternion_t *sourceQuat = (beacon_Quaternion_t *)lightSource->orientation;
        beacon_RenderQuaternion_t quat = {sourceQuat->x, sourceQuat->y, sourceQuat->z, sourceQuat->w};
        orientationMat = beacon_RenderMatrix3x3_fromQuaternion(quat);
    }
    else if(beacon_getClass(context, lightSource->orientation) == context->classes.matrix3x3Class)
    {
        beacon_Matrix3x3_t *sourceMat = (beacon_Matrix3x3_t*)lightSource->orientation;
        beacon_RenderMatrix3x3_t mat = {
            .m11 = sourceMat->m11, .m12 = sourceMat->m12, .m13 = sourceMat->m13,
            .m21 = sourceMat->m21, .m22 = sourceMat->m22, .m23 = sourceMat->m23,
            .m31 = sourceMat->m31, .m32 = sourceMat->m32, .m33 = sourceMat->m33,
        };

        orientationMat = mat;
    }
    else
    {
        BeaconAssert(context, false && "Unsupported light source orientation type.");
    }

    beacon_RenderVector3_t localLookDirection = {0, 0, 1};
    beacon_RenderVector3_t lookDirection = beacon_RenderMatrix3x3_multiplyVector(orientationMat, localLookDirection);

    beacon_RenderVector4_t positionOrDirection = {lightSource->position->x, lightSource->position->y, lightSource->position->z, 1};
    bool isDirectional = lightSource->isDirectional == context->roots.trueValue;
    if(isDirectional)
    {
        positionOrDirection.x = lookDirection.x; 
        positionOrDirection.y = lookDirection.y;
        positionOrDirection.z = lookDirection.z;
        positionOrDirection.w = 0;
    }

    beacon_RenderLightSource_t renderLightSource = {
        .positionOrDirection = positionOrDirection,
        .intensity = {intensity->x, intensity->y, intensity->z},
        .influenceRadius = influenceRadius,
        .spotDirection = {lookDirection.x, lookDirection.y, lookDirection.z},
        .innerSpotCosCutoff = cos(innerSpotCutoff * (M_PI / 180.0)),
        .outerSpotCosCutoff = cos(outerSpotCutoff * (M_PI / 180.0)),
        .castShadows = lightSource->castShadows == context->roots.trueValue,
    };

    bool castShadows = renderLightSource.castShadows;
    bool isSpot = innerSpotCutoff < 180.0 || outerSpotCutoff < 180.0;
    int shadowMapPartCount = 0;
    if(isDirectional)
    {
        renderLightSource.innerSpotCosCutoff = -1;
        renderLightSource.outerSpotCosCutoff = -1;
        if(castShadows)
        {
            shadowMapPartCount = 4;
            renderLightSource.shadowMapNormalBiasFactor = beacon_decodeNumberAsDouble(context, lightSource->shadowMapNormalBiasFactor);
            beacon_RenderCameraState_t cameraRenderState = {};
            if(agpu->cameraState.size > 0)
                cameraRenderState = ((beacon_RenderCameraState_t*)agpu->cameraState.thisFrameBuffer)[agpu->cameraState.size - 1];

            shadowMapPartCount = 4;
            int numSlices = 4;
            float splitDistributionDistances[5];
            for(int i = 0; i <= shadowMapPartCount; ++i)
            {
                // Cascade shadow map split distribution scheme from GPU Gems 3, Chapter 10:
                // Parallel-Split Shadow Maps on Programmable GPUs

                float uniformSlice = (cameraRenderState.farDistance - cameraRenderState.nearDistance) / (float)(numSlices)*i + cameraRenderState.nearDistance;
                float exponentialSlice = cameraRenderState.nearDistance * powf(cameraRenderState.farDistance / cameraRenderState.nearDistance, (float)(i)/numSlices);
                float cascadeDistributionLambda = 0.99;
                float split = (1.0-cascadeDistributionLambda)*uniformSlice + cascadeDistributionLambda*exponentialSlice;
                splitDistributionDistances[i] = split;
            }

            float splitDistributionLambdas[5];
            for(int i = 0; i <= 4; ++i)
            {
                float distance = splitDistributionDistances[i];
                splitDistributionLambdas[i] = (distance - cameraRenderState.nearDistance) / (cameraRenderState.farDistance - cameraRenderState.nearDistance);
                if(splitDistributionLambdas[i] < 0.0f)
                    splitDistributionLambdas[i] = 0.0f;
                else if(splitDistributionLambdas[i] > 1.0f)
                    splitDistributionLambdas[i] = 1.0f;
            }

            beacon_RenderVector4_t shadowMapCascadeDistanceWorldTransform = {
                -cameraRenderState.viewMatrix.m31, -cameraRenderState.viewMatrix.m32, -cameraRenderState.viewMatrix.m33, -cameraRenderState.viewMatrix.m34
            };

            beacon_RenderVector4_t shadowMapCascadeOffsets = {
                splitDistributionDistances[1], splitDistributionDistances[2], splitDistributionDistances[3], splitDistributionDistances[4]
            };

            renderLightSource.shadowMapCascadeDistanceWorldTransform = shadowMapCascadeDistanceWorldTransform;
            renderLightSource.shadowMapCascadeOffsets = shadowMapCascadeOffsets;

            for(int i = 0; i < shadowMapPartCount; ++i)
            {
                beacon_Frustum_t frustumSlice = beacon_Frustum_splitAtNearAndFarLambda(&cameraRenderState.worldFrustum, splitDistributionLambdas[i], splitDistributionLambdas[i + 1]);
                beacon_RenderVector3_t frustumCenter = beacon_AABox3_center(&frustumSlice.boundingBox);
                beacon_RenderMatrix4x4_t cascadeTransform = beacon_RenderMatrix4x4_withMatrix3x3AndTranslation(orientationMat, frustumCenter);

                renderLightSource.modelMatrix[i] = cascadeTransform;
                renderLightSource.inverseModelMatrix[i] = beacon_RenderMatrix4x4_inverse(cascadeTransform);

                beacon_RenderVector3_t worldCorners[8] = {
                    frustumSlice.leftBottomNear,
                    frustumSlice.rightBottomNear,
                    frustumSlice.leftTopNear,
                    frustumSlice.rightTopNear,

                    frustumSlice.leftBottomFar,
                    frustumSlice.rightBottomFar,
                    frustumSlice.leftTopFar,
                    frustumSlice.rightTopFar,
                };

                beacon_AABox3_t localShadowVolume = beacon_AABox3_empty();
                for(size_t c = 0; c < 8; ++c)
                {
                    beacon_RenderVector3_t localPoint = beacon_RenderMatrix4x4_multiplyVector3(renderLightSource.inverseModelMatrix[i], worldCorners[c]);
                    beacon_AABox3_insertPoint(&localShadowVolume, localPoint);
                }

                float influenceRadius = 100.0f;
                localShadowVolume.min.z -= influenceRadius;
                localShadowVolume.max.z += influenceRadius;

                renderLightSource.projectionMatrix[i] = beacon_RenderMatrix4x4_reverseDepthOrtho(
                    localShadowVolume.min.x, localShadowVolume.max.x,
                    localShadowVolume.min.y, localShadowVolume.max.y,
                    -localShadowVolume.max.z, -localShadowVolume.min.z);
                renderLightSource.inverseProjectionMatrix[i] = beacon_RenderMatrix4x4_inverse(renderLightSource.projectionMatrix[i]);
            }
        }
    }
    else if(isSpot)
    {
        if(castShadows)
        {
            shadowMapPartCount = 1;
            beacon_RenderVector3_t translation = {lightSource->position->x, lightSource->position->y, lightSource->position->z};
            beacon_RenderMatrix4x4_t modelMatrix = beacon_RenderMatrix4x4_withMatrix3x3AndTranslation(orientationMat, translation);
            beacon_RenderMatrix4x4_t inverseModelMatrix = beacon_RenderMatrix4x4_inverse(modelMatrix);

            renderLightSource.shadowMapNormalBiasFactor = beacon_decodeNumberAsDouble(context, lightSource->shadowMapNormalBiasFactor);
            renderLightSource.modelMatrix[0] = modelMatrix;
            renderLightSource.inverseModelMatrix[0] = inverseModelMatrix;
            renderLightSource.projectionMatrix[0] = beacon_RenderMatrix4x4_reverseDepthPerspective(outerSpotCutoff*2.0, 1.0, 0.01, renderLightSource.influenceRadius, flipProjectionVertically);
            renderLightSource.inverseProjectionMatrix[0] = beacon_RenderMatrix4x4_inverse(renderLightSource.projectionMatrix[0]);
        }
    }
    else // Point
    {
        renderLightSource.innerSpotCosCutoff = -1;
        renderLightSource.outerSpotCosCutoff = -1;
        if(renderLightSource.castShadows)
        {
            shadowMapPartCount = 6;
            beacon_RenderVector3_t center = {renderLightSource.positionOrDirection.x, renderLightSource.positionOrDirection.y, renderLightSource.positionOrDirection.z};

            renderLightSource.shadowMapNormalBiasFactor = beacon_decodeNumberAsDouble(context, lightSource->shadowMapNormalBiasFactor);
            renderLightSource.projectionMatrix[0] = beacon_RenderMatrix4x4_reverseDepthPerspective(90, 1.0, 0.1, renderLightSource.influenceRadius, flipProjectionVertically);
            renderLightSource.inverseProjectionMatrix[0] = beacon_RenderMatrix4x4_inverse(renderLightSource.projectionMatrix[0]) ;

            for(int i = 0; i < shadowMapPartCount; ++i)
            {
                beacon_RenderMatrix3x3_t faceMatrix = beacon_RenderMatrix3x3_CubeMapFaceRotations[i];
                renderLightSource.modelMatrix[i] = beacon_RenderMatrix4x4_withMatrix3x3AndTranslation(faceMatrix, center);
                renderLightSource.inverseModelMatrix[i] = beacon_RenderMatrix4x4_inverse(renderLightSource.modelMatrix[i]);

                renderLightSource.projectionMatrix[i] = renderLightSource.projectionMatrix[0];
                renderLightSource.inverseProjectionMatrix[i] = renderLightSource.inverseProjectionMatrix[0];

            }
        }
    }
    
    if(renderLightSource.castShadows)
    {
        beacon_AGPUShadowCastingLight_t shadowCastingLight = {
            .renderLightSourceIndex = agpu->renderLightSourceAttributes.size,
            .shadowMapPartCount = shadowMapPartCount
        };

        bool isMissingPiece = false;
        for(int i = 0; i < shadowMapPartCount; ++i)
        {
            if(!shadowMapAtlasAllocator_allocate(&agpu->shadowMapAtlasAllocator, shadowCastingLight.atlasAllocations + i))
            {
                isMissingPiece = true;
                renderLightSource.castShadows = false;
                break;
            }
        }

        if(!isMissingPiece && agpu->numberOfShadowCastingLightSources < BEACON_AGPU_MAX_SHADOW_CASTING_LIGHTS)
        {
            beacon_RenderVector2_t viewportScale = {0.5f, 0.5f};
            if(flipTextureVertically)
                viewportScale.y = -viewportScale.y;
            beacon_RenderVector2_t viewportOffset = {0.5f, 0.5f}; 

            beacon_RenderVector2_t atlasExtent =  shadowCastingLight.atlasAllocations[0].shadowMapAtlasExtent;
            beacon_RenderVector2_t viewportExtent = shadowCastingLight.atlasAllocations[0].shadowMapExtent;
            beacon_RenderVector2_t viewportExtentScale = {viewportExtent.x / atlasExtent.x, viewportExtent.y / atlasExtent.y};

            beacon_RenderVector2_t shadowMapViewportScale = {viewportScale.x *viewportExtentScale.x, viewportScale.y *viewportExtentScale.y};
            renderLightSource.shadowMapViewportScale = shadowMapViewportScale;

            for(int i = 0; i < shadowMapPartCount; ++i)
            {
                renderLightSource.shadowMapViewportOffsets[i].x = viewportOffset.x*viewportExtentScale.x + shadowCastingLight.atlasAllocations[i].offset.x / atlasExtent.x;
                renderLightSource.shadowMapViewportOffsets[i].y = viewportOffset.y*viewportExtentScale.y + shadowCastingLight.atlasAllocations[i].offset.y / atlasExtent.y;
            }

            agpu->shadowCastingLightSources[agpu->numberOfShadowCastingLightSources++] = shadowCastingLight;
        }
    }

    beacon_agpu_pushRenderLightSource(agpu, renderLightSource);
    return receiver;
}

void beacon_context_registerAgpuRenderingPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.agpuClass), "uniqueInstance", 1, beacon_AGPU_uniqueInstance);

    beacon_addPrimitiveToClass(context, context->classes.agpuClass, "platform", 1, beacon_AGPU_platform);
    beacon_addPrimitiveToClass(context, context->classes.agpuClass, "device", 1, beacon_AGPU_device);

    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "beginFrame", 0, beacon_agpuWindowRenderer_beginFrame);
    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "renderQuadList:", 0, beacon_agpuWindowRenderer_renderQuadList);
    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "endFrame", 0, beacon_agpuWindowRenderer_endFrame);

    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "begin3DFrameRenderingWithWidth:height:", 2, beacon_agpuWindowRenderer_begin3DFrameRendering);
    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "end3DFrameRendering", 0, beacon_agpuWindowRenderer_end3DFrameRendering);

    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "addSceneCamera:", 1, beacon_agpuWindowRenderer_addSceneCamera);
    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "addRenderObject:", 1, beacon_agpuWindowRenderer_addRenderObject);
    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "addLightSource:", 1, beacon_agpuWindowRenderer_addLightSource);
    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "get3DOutputTextureHandle", 0, beacon_agpuWindowRenderer_get3DOutputTextureHandle);
}

