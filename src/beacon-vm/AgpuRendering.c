
#include "AgpuRendering.h"
#include "Exceptions.h"
#include <stdio.h>
#include <stdlib.h>
#include "stb_truetype.h"

typedef struct VulkanDrawIndexedIndirectCommand {
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    int32_t     vertexOffset;
    uint32_t    firstInstance;
} VulkanDrawIndexedIndirectCommand;

typedef VulkanDrawIndexedIndirectCommand DrawIndirectCommand;

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

void beacon_agpu_loadPipelineStates(beacon_context_t *context, beacon_AGPU_t *agpu)
{
    agpu_device *device = agpu->device;
    bool hasTextureInvertedProjectionY = agpuHasTopLeftNdcOrigin(device) == agpuHasBottomLeftTextureCoordinates(device);

    agpu_shader *screenQuadShader;
    if (hasTextureInvertedProjectionY)
        screenQuadShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "GuiVertex", NULL, "scripts/runtime/assets/shaders/ScreenQuadFlippedY.glsl", AGPU_VERTEX_SHADER);
    else
        screenQuadShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "GuiVertex", NULL, "scripts/runtime/shaders/ScreenQuad.glsl", AGPU_VERTEX_SHADER);

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
    initializeUpdateBuffer(&agpu->renderLightSourceAttributes, &agpu->renderMaterialsAttributes, sizeof(beacon_RenderMaterialAttributes_t), BEACON_AGPU_MAX_MATERIALS);

    initializeUpdateBuffer(&agpu->vertexPositions,   &agpu->renderLightSourceAttributes, 3*sizeof(float), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexNormals,     &agpu->vertexPositions,             3*sizeof(float), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexTexcoords,   &agpu->vertexNormals,               2*sizeof(float), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexTangent4,    &agpu->vertexTexcoords,             4*sizeof(float), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexBoneIndices, &agpu->vertexTangent4,              4*sizeof(uint16_t), BEACON_AGPU_MAX_VERTICES);
    initializeUpdateBuffer(&agpu->vertexBoneWeights, &agpu->vertexBoneIndices,           4*sizeof(float), BEACON_AGPU_MAX_VERTICES);

    initializeUpdateBuffer(&agpu->guiData, &agpu->vertexBoneWeights, sizeof(beacon_GuiRenderingElement_t), BEACON_AGPU_MAX_NUMBER_OF_QUADS);

    initializeUpdateBuffer(&agpu->cameraState, &agpu->guiData, sizeof(beacon_RenderCameraState_t), BEACON_AGPU_MAX_CAMERA_STATES);

    initializeUpdateBuffer(&agpu->indexData, &agpu->guiData, sizeof(uint32_t), BEACON_AGPU_MAX_INDICES);

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
    agpuBindStorageBufferRange(agpu->renderingDataBinding, 3,  agpu->gpu3DRenderingDataBuffer, agpu->vertexNormals.offset, agpu->vertexNormals.byteCapacity);
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
    size_t frameRenderingDataSize = context->roots.agpuCommon->guiData.endOffset;
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

    if(thisFrameState->hasSubmittedToQueue)
    {
        agpuWaitOnClient(thisFrameState->fence);
        thisFrameState->hasSubmittedToQueue = false;
    }

    agpuResetCommandAllocator(thisFrameState->commandAllocator);
    agpuResetCommandList(thisFrameState->commandList, thisFrameState->commandAllocator, NULL);

    uint8_t *renderingDataUploadBuffer = renderer->renderingDataUploadBuffer + renderer->frameRenderingDataUploadSize* renderer->currentFrameBufferingIndex;
    beacon_agpuWindowRenderer_resetPerFrameBuffers(context, renderer, thisFrameState);

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
        if(renderer->hasIntermediateBuffers)
        {
            agpuFinishDeviceExecution(device);
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

            renderer->depthOnlyFramebuffer = NULL;
            renderer->hdrOpaqueFramebuffer = NULL;
            renderer->hdrFramebuffer       = NULL;

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
    }

    if(!renderer->intermediateBindings)
        renderer->intermediateBindings = agpuCreateShaderResourceBinding(context->roots.agpuCommon->shaderSignature, 3);
    {
        agpu_texture_view *textureView = agpuGetOrCreateFullTextureView(renderer->hdrColorBuffer);
        agpuBindSampledTextureView(renderer->intermediateBindings, 0, textureView);
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

static beacon_oop_t beacon_agpuWindowRenderer_end3DFrameRendering(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t*)receiver;
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

    // Perform the main geometry culling
    uint32_t renderObjectsSize = agpu->renderObjectAttributes.size;
    uint32_t lightSourceCount = agpu->renderLightSourceAttributes.size;
    uint32_t nullPushConstant = 0;
    
    agpuPushConstants(commandList, 0, 4, &renderObjectsSize);
    agpuPushConstants(commandList, 4, 4, &lightSourceCount);
    agpuPushConstants(commandList, 8, 4, &nullPushConstant);
    agpuPushConstants(commandList, 12, 4, &nullPushConstant);
    
    // Depth only render pass
    agpuBeginRenderPass(commandList, renderer->mainDepthRenderPass, renderer->depthOnlyFramebuffer, false);
    agpuSetViewport(commandList, 0, 0, displayWidth, displayHeight);
    agpuSetScissor(commandList, 0, 0, displayWidth, displayHeight);
    
    agpuUsePipelineState(commandList, agpu->opaqueDepthOnlyPipeline);
    
    // TODO: Render the opaque objects with color.

    agpuEndRenderPass(commandList);

    // Opaque color
    agpuBeginRenderPass(commandList, renderer->mainDepthColorOpaqueRenderPass, renderer->hdrOpaqueFramebuffer, false);
    agpuSetViewport(commandList, 0, 0, displayWidth, displayHeight);
    agpuSetScissor(commandList, 0, 0, displayWidth, displayHeight);

    // Draw the sky and the background.
    agpuUsePipelineState(commandList, agpu->daySkyPipeline);
    agpuDrawArrays(commandList, 3, 1, 0, 0);

    // TODO: Render the opaque objects with color.

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
    
    return receiver;
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

    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "get3DOutputTextureHandle", 0, beacon_agpuWindowRenderer_get3DOutputTextureHandle);
    

}

