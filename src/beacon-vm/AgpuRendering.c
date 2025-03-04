
#include "AgpuRendering.h"
#include <stdio.h>
#include <stdlib.h>

#define stringify_(value) #value
#define stringify(...) stringify_(__VA_ARGS__)

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

static agpu_shader *beacon_agpu_compileShaderWithSourceFileNamed(beacon_context_t *context, beacon_AGPU_t *agpu, const char *name, const char *sourceFileName, agpu_shader_type shaderType)
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

    agpu_shader *shader = beacon_agpu_compileShaderWithSource(context, agpu, name, shaderSource, shaderType);
    free(shaderSource);
    return shader;
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
        
        agpuBeginShaderSignatureBindingBank(builder, 1);
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_SAMPLER, 1);

        agpuBeginShaderSignatureBindingBank(builder, 128);
        agpuAddShaderSignatureBindingBankElement(builder, AGPU_SHADER_BINDING_TYPE_STORAGE_BUFFER, 1);

        agpuBeginShaderSignatureBindingBank(builder, 1);
        agpuAddShaderSignatureBindingBankArray(builder, AGPU_SHADER_BINDING_TYPE_SAMPLED_IMAGE, 1024);

        agpuAddShaderSignatureBindingConstant(builder); // hasTopLeftNDCOrigin
        agpuAddShaderSignatureBindingConstant(builder); // reservedConstant
        agpuAddShaderSignatureBindingConstant(builder); // framebufferReciprocalExtentX
        agpuAddShaderSignatureBindingConstant(builder); // framebufferReciprocalExtentY

        agpu->shaderSignature = agpuBuildShaderSignature(builder);
    }

    // Sampler and its binding
    {
        agpu_sampler_description samplerDesc = {
            .address_u = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .address_v = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .address_w = AGPU_TEXTURE_ADDRESS_MODE_WRAP,
            .filter = AGPU_FILTER_MIN_LINEAR_MAG_LINEAR_MIPMAP_NEAREST,
            .max_lod = 32
        };

        agpu->linearSampler = agpuCreateSampler(agpu->device, &samplerDesc);
        agpu->linearSamplerBinding = agpuCreateShaderResourceBinding(agpu->shaderSignature, 0);
    }

    // Uber GUI pipeline state
    {
        //printf("guiVertexShaderSource: %s\n", guiVertexShaderSource);
        //printf("guiFragmentShaderSource: %s\n", guiFragmentShaderSource);

        agpu_shader *vertexShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "GuiVertex", "scripts/runtime/shaders/GuiVertexShader.glsl", AGPU_VERTEX_SHADER);
        agpu_shader *fragmentShader = beacon_agpu_compileShaderWithSourceFileNamed(context, agpu, "GuiFragment", "scripts/runtime/shaders/GuiFragmentShader.glsl", AGPU_FRAGMENT_SHADER);

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
        agpu->texturesArrayBinding = agpuCreateShaderResourceBinding(agpu->shaderSignature, 2);
    }
}

beacon_AGPUWindowRenderer_t *beacon_agpu_createWindowRenderer(beacon_context_t *context)
{
    agpu_device *device = beacon_agpu_getDevice(context, context->roots.agpuCommon);
    if(!device)
        return NULL;

    if(!context->roots.agpuCommon->shaderSignature)
        beacon_agpu_initializeCommonObjects(context, context->roots.agpuCommon);

    beacon_AGPUWindowRenderer_t *windowRenderer = beacon_allocateObjectWithBehavior(context->heap, context->classes.agpuWindowRendererClass, sizeof(beacon_AGPUWindowRenderer_t), BeaconObjectKindBytes);
    windowRenderer->commandQueue = agpuGetDefaultCommandQueue(device);
    windowRenderer->frameRenderingDataSize = BEACON_AGPU_FRAMEBUFFERING_COUNT * BEACON_AGPU_MAX_NUMBER_OF_QUADS * sizeof(beacon_GuiRenderingElement_t);

    {
        agpu_buffer_description desc = {
            .heap_type = AGPU_MEMORY_HEAP_TYPE_DEVICE_LOCAL,
            .usage_modes = AGPU_COPY_DESTINATION_BUFFER | AGPU_STORAGE_BUFFER,
            .main_usage_mode = AGPU_STORAGE_BUFFER,
            .size = windowRenderer->frameRenderingDataSize,
            .stride = sizeof(beacon_GuiRenderingElement_t)
        };
        windowRenderer->renderingDataGpuBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    {
        agpu_buffer_description desc = {
            .heap_type = AGPU_MEMORY_HEAP_TYPE_HOST,
            .usage_modes = AGPU_COPY_SOURCE_BUFFER,
            .main_usage_mode = AGPU_COPY_SOURCE_BUFFER,
            .size = windowRenderer->frameRenderingDataSize,
            .stride = sizeof(beacon_GuiRenderingElement_t),
            .mapping_flags = AGPU_MAP_WRITE_BIT
        };
        windowRenderer->renderingDataSubmissionBuffer = agpuCreateBuffer(device, &desc, NULL);
    }

    windowRenderer->renderingDataUploadBuffer = agpuMapBuffer(windowRenderer->renderingDataSubmissionBuffer, AGPU_WRITE_ONLY);

    for(int i = 0; i < BEACON_AGPU_FRAMEBUFFERING_COUNT; ++i)
    {
        beacon_AGPUWindowRendererPerFrameState_t *frameState = windowRenderer->frameState + i;
        frameState->fence = agpuCreateFence(device);
        frameState->commandAllocator = agpuCreateCommandAllocator(device, AGPU_COMMAND_LIST_TYPE_DIRECT, windowRenderer->commandQueue);
        agpuCloseCommandList(frameState->commandList);

        frameState->commandList = agpuCreateCommandList(device, AGPU_COMMAND_LIST_TYPE_DIRECT, windowRenderer->frameState[i].commandAllocator, NULL);
        frameState->renderingDataBinding = agpuCreateShaderResourceBinding(context->roots.agpuCommon->shaderSignature, 1);
        size_t bufferSize = BEACON_AGPU_FRAMEBUFFERING_COUNT*sizeof(beacon_GuiElementType_t);
        agpuBindStorageBufferRange(frameState->renderingDataBinding, 0, windowRenderer->renderingDataGpuBuffer, bufferSize*i, bufferSize);
        frameState->renderingDataUploadBuffer = windowRenderer->renderingDataUploadBuffer + (BEACON_AGPU_MAX_NUMBER_OF_QUADS*i);
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

    thisFrameState->renderingDataUploadSize = 0;

    printf("Begin frame %d.\n", renderer->currentFrameBufferingIndex);
    return receiver;
}

static beacon_oop_t beacon_agpuWindowRenderer_endFrame(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t*)receiver;
    beacon_AGPUWindowRendererPerFrameState_t *thisFrameState = renderer->frameState + renderer->currentFrameBufferingIndex;

    {
        size_t offset = BEACON_AGPU_MAX_NUMBER_OF_QUADS*sizeof(beacon_GuiRenderingElement_t)*renderer->currentFrameBufferingIndex;
        size_t size = thisFrameState->renderingDataUploadSize*sizeof(beacon_GuiRenderingElement_t);
        agpuCopyBuffer(thisFrameState->commandList,
            renderer->renderingDataSubmissionBuffer, offset,
            renderer->renderingDataGpuBuffer, offset,
            size);
    }

    agpuSetShaderSignature(thisFrameState->commandList, context->roots.agpuCommon->shaderSignature);
    agpuUseShaderResources(thisFrameState->commandList, context->roots.agpuCommon->linearSamplerBinding);
    agpuUseShaderResources(thisFrameState->commandList, thisFrameState->renderingDataBinding);
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
        agpuPushConstants(thisFrameState->commandList, 0, 4, &hasTopLeftNDCOriginValue);

        float framebufferReciprocalExtentX = 1.0f / screenWidth;
        float framebufferReciprocalExtentY = 1.0f / screenHeight;
        agpuPushConstants(thisFrameState->commandList, 8, 4, &framebufferReciprocalExtentX);
        agpuPushConstants(thisFrameState->commandList, 12, 4, &framebufferReciprocalExtentY);
    }

    agpuUsePipelineState(thisFrameState->commandList, context->roots.agpuCommon->guiPipelineState);
    agpuDrawArrays(thisFrameState->commandList, 4, thisFrameState->renderingDataUploadSize, 0, 0);

    // End the main buffer renderpass.
    agpuEndRenderPass(thisFrameState->commandList);

    agpuCloseCommandList(thisFrameState->commandList);
    agpuAddCommandListsAndSignalFence(renderer->commandQueue, 1, &thisFrameState->commandList, thisFrameState->fence);
    thisFrameState->hasSubmittedToQueue = true;
    agpuSwapBuffers(renderer->swapChain->swapChain);

    agpuReleaseFramebuffer(backbuffer);
    printf("End frame.\n");
    
    renderer->currentFrameBufferingIndex = (renderer->currentFrameBufferingIndex + 1) % BEACON_AGPU_FRAMEBUFFERING_COUNT;
    return receiver;
}

static beacon_oop_t beacon_agpuWindowRenderer_renderQuadList(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_AGPUWindowRenderer_t *renderer = (beacon_AGPUWindowRenderer_t*)receiver;
    beacon_AGPUWindowRendererPerFrameState_t *thisFrameState = renderer->frameState + renderer->currentFrameBufferingIndex;

    beacon_Array_t *quadList = (beacon_Array_t*)arguments[0];
    size_t quadCount = quadList->super.super.super.super.super.header.slotCount;
    printf("TODO: render quad-list: %d\n", (int)quadCount);
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

            thisFrameState->renderingDataUploadBuffer[thisFrameState->renderingDataUploadSize++] = quad;

        }
        else if(quadClass == context->classes.formTextRenderingElementClass)
        {
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
    //beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.agpuClass), "device", 1, beacon_AGPU_device);

    beacon_addPrimitiveToClass(context, context->classes.agpuClass, "platform", 1, beacon_AGPU_platform);
    beacon_addPrimitiveToClass(context, context->classes.agpuClass, "device", 1, beacon_AGPU_device);

    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "beginFrame", 0, beacon_agpuWindowRenderer_beginFrame);
    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "renderQuadList:", 0, beacon_agpuWindowRenderer_renderQuadList);
    beacon_addPrimitiveToClass(context, context->classes.agpuWindowRendererClass, "endFrame", 0, beacon_agpuWindowRenderer_endFrame);
}

