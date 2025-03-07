#ifndef BEACON_AGPU_RENDERING_H
#define BEACON_AGPU_RENDERING_H

#include "AGPU/agpu.h"
#include "Context.h"
#include "ObjectModel.h"
#include "Math.h"

#define BEACON_AGPU_FRAMEBUFFERING_COUNT 3
#define BEACON_AGPU_MAX_NUMBER_OF_QUADS 1024
#define BEACON_AGPU_TEXTURE_ARRAY_SIZE 1024

#define BEACON_AGPU_MAX_MATERIALS 1024
#define BEACON_AGPU_MAX_RENDER_OBJECTS 1024
#define BEACON_AGPU_MAX_MESHES 1024
#define BEACON_AGPU_MAX_MODELS 1024
#define BEACON_AGPU_MAX_LIGHT_SOURCES 1024
#define BEACON_AGPU_MAX_VERTICES (1024*1024)
#define BEACON_AGPU_MAX_INDICES (1024*1024)

#define BEACON_AGPU_MAX_LIGHT_CLUSTER_CAPACITY 100
#define BEACON_AGPU_SHADOW_MAP_ATLAS_SIZE 4096

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

typedef struct beacon_RenderObjectAttributes_s
{
    // If positive, this is a free slot that should not be rendered.
    beacon_RenderMatrix4x4_t modelMatrix;
    beacon_RenderMatrix4x4_t inverseModelMatrix;

    int32_t modelIndex;
}beacon_RenderObjectAttributes_t;

typedef struct beacon_RenderModelAttributes_s
{
    uint32_t submeshCount;
    int32_t firstSubmeshIndex;
} beacon_RenderModelAttributes_t;

typedef struct beacon_RenderMeshPrimitiveAttributes_s
{
    int32_t materialIndex;

    uint32_t vertexCount;
    int32_t firstPositionIndex;
    int32_t firstNormalIndex;
    int32_t firstTangents4Index;
    int32_t firstTexcoordIndex;

    uint32_t indexCount;
    int32_t firstIndexPosition;
} beacon_RenderMeshPrimitiveAttributes_t;

typedef struct beacon_RenderMaterialAttributes_s
{
    float emissiveColor[3];
    float padding;
    float baseColor;
    
    float metallicFactor;
    float roughnessFactor;
    float occlusionFactor;
    uint32_t isTranslucent;
    
    uint32_t baseColorTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t emissiveTextureIndex;
    uint32_t roughnessMetallicTextureIndex;
    uint32_t occlusionTextureIndex;
} beacon_RenderMaterialAttributes_t;

typedef struct beacon_RenderLightSource_s
{
	beacon_RenderVector4_t positionOrDirection;

	beacon_RenderVector3_t intensity;
	float influenceRadius;

	beacon_RenderVector3_t spotDirection;
	float innerSpotCosCutoff;

	float outerSpotCosCutoff;
	bool castShadows;
	beacon_RenderVector2_t shadowMapViewportScale;
	
	float shadowMapNormalBiasFactor;

	beacon_RenderVector4_t shadowMapCascadeDistanceWorldTransform;
	beacon_RenderVector4_t shadowMapCascadeOffsets;
	
	beacon_RenderMatrix4x4_t modelMatrix;
	beacon_RenderMatrix4x4_t inverseModelMatrix;

	beacon_RenderMatrix4x4_t projectionMatrix;
	beacon_RenderMatrix4x4_t inverseProjectionMatrix;

	beacon_RenderVector2_t shadowMapViewportOffsets;
}beacon_RenderLightSource_t;

typedef struct beacon_AGPUUpdateBuffer_s
{
    size_t elementSize;
    size_t capacity;
    size_t size;
    size_t byteCapacity;
    size_t offset;
    size_t endOffset;
} beacon_AGPUUpdateBuffer_t;

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

    bool updateBuffersCreated;
    beacon_AGPUUpdateBuffer_t renderObjectAttributes;
    beacon_AGPUUpdateBuffer_t renderModelAttributes;
    beacon_AGPUUpdateBuffer_t renderMeshPrimitiveAttributes;
    beacon_AGPUUpdateBuffer_t renderMaterialsAttributes;
    beacon_AGPUUpdateBuffer_t renderLightSourceAttributes;

    beacon_AGPUUpdateBuffer_t vertexPositions;
    beacon_AGPUUpdateBuffer_t vertexNormals;
    beacon_AGPUUpdateBuffer_t vertexTexcoords;
    beacon_AGPUUpdateBuffer_t vertexTangent4;
    beacon_AGPUUpdateBuffer_t vertexBoneIndices;
    beacon_AGPUUpdateBuffer_t vertexBoneWeights;

    beacon_AGPUUpdateBuffer_t guiData;

    beacon_AGPUUpdateBuffer_t indexData;

    agpu_buffer *gpu3DRenderingDataBuffer;
    agpu_buffer *gpu3DRenderingIndexBuffer;

    agpu_shader_resource_binding *renderingDataBinding;

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
    uint8_t *renderingDataUploadBuffer;
    beacon_GuiRenderingElement_t *guiRenderingDataUploadBuffer;
    size_t guiRenderingDataUploadSize;
    bool hasSubmittedToQueue;
} beacon_AGPUWindowRendererPerFrameState_t;

typedef struct beacon_AGPUWindowRenderer_s
{
    beacon_Object_t super;
    int currentFrameBufferingIndex;

    agpu_buffer *renderingDataSubmissionBuffer;
    size_t frameRenderingDataUploadSize;
    uint8_t *renderingDataUploadBuffer;

    agpu_command_queue *commandQueue;
    beacon_AGPUSwapChain_t *swapChain;
    beacon_AGPUWindowRendererPerFrameState_t frameState[BEACON_AGPU_FRAMEBUFFERING_COUNT];
} beacon_AGPUWindowRenderer_t;

agpu_platform *beacon_agpu_getPlatform(beacon_context_t *context, beacon_AGPU_t *agpu);
agpu_device *beacon_agpu_getDevice(beacon_context_t *context, beacon_AGPU_t *agpu);

beacon_AGPUWindowRenderer_t *beacon_agpu_createWindowRenderer(beacon_context_t *context);
void beacon_agpu_destroyWindowRenderer(beacon_context_t *context, beacon_AGPUWindowRenderer_t *renderer);

#endif // BEACON_AGPU_RENDERING_H