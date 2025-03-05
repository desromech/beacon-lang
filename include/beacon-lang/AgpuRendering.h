#ifndef BEACON_AGPU_RENDERING_H
#define BEACON_AGPU_RENDERING_H

#include "AGPU/agpu.h"
#include "Context.h"
#include "ObjectModel.h"

#define BEACON_AGPU_FRAMEBUFFERING_COUNT 3
#define BEACON_AGPU_MAX_NUMBER_OF_QUADS 1024
#define BEACON_AGPU_TEXTURE_ARRAY_SIZE 1024

typedef enum beacon_GuiElementType_e
{
    BeaconGuiSolidRectangle,
    BeaconGuiHorizontalGradient,
    BeaconGuiVerticalGradient,
    BeaconGuiTextCharacter,
}beacon_GuiElementType_t;

typedef struct beacon_GuiRenderingElement_s
{
    uint32_t type;
    int32_t texture;
    float borderRoundRadius;
    float borderSize;

    float rectangleMinX, rectangleMinY;
    float rectangleMaxX, rectangleMaxY;

    float imageRectangleMinX, imageRectangleMinY;
    float imageRectangleMaxX, imageRectangleMaxY;

    float firstColor[4];
    float secondColor[4];
    float borderColor[4];
} beacon_GuiRenderingElement_t;

typedef struct beacon_AGPU_s
{
    beacon_Object_t super;
    uint32_t platformIndex;
    agpu_platform *platform;

    uint32_t deviceIndex;
    agpu_device *device;

    bool debugLayerEnabled;

    agpu_shader_signature *shaderSignature;
    agpu_renderpass *mainRenderPass;

    agpu_sampler *linearSampler;
    agpu_shader_resource_binding *linearSamplerBinding;
    agpu_pipeline_state *guiPipelineState;

    agpu_texture *errorTexture;
    agpu_texture *boundTextures[1024];
    agpu_texture_view *boundTextureViews[1024];

    int textureArrayBindingCount;
    agpu_shader_resource_binding *texturesArrayBinding;

} beacon_AGPU_t;

typedef struct beacon_AGPUTextureHandle_s
{
    beacon_Object_t super;
    agpu_texture *texture;
    agpu_texture_view *textureView;
    int textureArrayBindingIndex;
} beacon_AGPUTextureHandle_t;

typedef struct beacon_AGPUSwapChain_s
{
    beacon_Object_t super;
    agpu_command_queue *commandQueue;
    agpu_swap_chain *swapChain;
} beacon_AGPUSwapChain_t;

typedef struct beacon_AGPUWindowRendererPerFrameState_s
{
    beacon_Object_t super;
    agpu_fence *fence;
    agpu_command_allocator *commandAllocator;
    agpu_command_list *commandList;
    agpu_shader_resource_binding *renderingDataBinding;
    beacon_GuiRenderingElement_t *renderingDataUploadBuffer;
    size_t renderingDataUploadSize;
    bool hasSubmittedToQueue;
} beacon_AGPUWindowRendererPerFrameState_t;

typedef struct beacon_AGPUWindowRenderer_s
{
    beacon_Object_t super;
    int currentFrameBufferingIndex;
    size_t frameRenderingDataSize;
    agpu_buffer *renderingDataGpuBuffer;
    agpu_buffer *renderingDataSubmissionBuffer;
    beacon_GuiRenderingElement_t *renderingDataUploadBuffer;
    agpu_command_queue *commandQueue;
    beacon_AGPUSwapChain_t *swapChain;
    beacon_AGPUWindowRendererPerFrameState_t frameState[BEACON_AGPU_FRAMEBUFFERING_COUNT];
} beacon_AGPUWindowRenderer_t;

agpu_platform *beacon_agpu_getPlatform(beacon_context_t *context, beacon_AGPU_t *agpu);
agpu_device *beacon_agpu_getDevice(beacon_context_t *context, beacon_AGPU_t *agpu);

beacon_AGPUWindowRenderer_t *beacon_agpu_createWindowRenderer(beacon_context_t *context);
void beacon_agpu_destroyWindowRenderer(beacon_context_t *context, beacon_AGPUWindowRenderer_t *renderer);

#endif // BEACON_AGPU_RENDERING_H