#version 450

layout( push_constant ) uniform constants
{
	uint renderObjectsSize;
    uint lightSourceCount;
    uint shadowMapLightSourceIndex;
    uint shadowMapComponent;

    bool hasTopLeftNDCOrigin;
    uint cameraStateIndex;
    vec2 framebufferReciprocalExtent;
    
    uint hdrTextureIndex;
} PushConstants;

struct Frustum
{
	vec3 leftBottomNear;
	vec3 rightBottomNear;
	vec3 leftTopNear;
	vec3 rightTopNear;
	vec3 leftBottomFar;
	vec3 rightBottomFar;
	vec3 leftTopFar;
	vec3 rightTopFar;

    vec4 planes[6];
};

struct CameraState
{
    vec2 framebufferExtent;
    vec2 framebufferReciprocalExtent;

    uint flipVertically;
    float nearDistance;
    float farDistance;

    float timeOfSimulation;
    float timeOfDay;

    float exposure;

    vec3 ambientLightSource;

    vec2 lightGridDepthSliceScaleOffset;
	bool hasTopLeftNDCOrigin;
	bool hasBottomLeftTextureCoordinates;

    uvec3 lightGridExtent;

    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    mat4 viewMatrix;
    mat4 inverseViewMatrix;

    Frustum worldFrustum;
};

layout(std140, set=2, binding = 19) buffer CameraStateBlock {
    CameraState cameraStates[];
};

struct RenderLightSource
{
	vec4 positionOrDirection;

	vec3 intensity;
	float influenceRadius;

	vec3 spotDirection;
	float innerSpotCosCutoff;

	float outerSpotCosCutoff;
	bool castShadows;
	vec2 shadowMapViewportScale;
	
	float shadowMapNormalBiasFactor;
	vec4 shadowMapCascadeDistanceWorldTransform;
	vec4 shadowMapCascadeOffsets;

	mat4 modelMatrix[6];
	mat4 inverseModelMatrix[6];

	mat4 projectionMatrix[6];
	mat4 inverseProjectionMatrix[6];

	vec2 shadowMapViewportOffsets[6];
};

struct RenderObjectAttributes
{
    mat4 modelMatrix;
    mat4 inverseModelMatrix;

    int modelIndex;
};

struct RenderModelAttributes
{
    uint submeshCount;
    int firstSubmeshIndex;
};

struct RenderMeshPrimitiveAttributes
{
    int materialIndex;

    uint vertexCount;
    int firstPositionIndex;
    int firstNormalIndex;
    int firstTangents4Index;
    int firstTexcoordIndex;

    uint indexCount;
    int firstIndexPosition;
};

struct RenderMaterialAttributes
{
    vec3 emissiveColor;
    vec4 baseColor;
    
    float metallicFactor;
    float roughnessFactor;
    float occlusionFactor;
    uint isTranslucent;
    
    int baseColorTextureIndex;
    int normalTextureIndex;
    int emissiveTextureIndex;
    int roughnessMetallicTextureIndex;
    int occlusionTextureIndex;
};

struct DrawIndexedIndirectCommand {
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    int     vertexOffset;
    uint    firstInstance;
};

layout(std430, set=2, binding = 0) buffer RenderObject
{
    RenderObjectAttributes RenderObjectData[];
};

layout(std430, set=2, binding = 1) buffer RenderModel
{
    RenderModelAttributes RenderModelData[];
};

layout(std430, set=2, binding = 2) buffer RenderSubmesh
{
    RenderMeshPrimitiveAttributes RenderSubmeshData[];
};

layout(std430, set=2, binding = 3) buffer RenderMaterial
{
    RenderMaterialAttributes RenderMaterialData[];
};

layout(std430, set=2, binding = 4) buffer WorldRenderLights
{
    RenderLightSource WorldRenderLightSourceData[];
};

layout(std430, set=2, binding = 5) buffer Positions
{
    vec3 PositionsData[];
};

layout(std430, set=2, binding = 6) buffer Normals
{
    vec3 NormalsData[];
};

layout(std430, set=2, binding = 7) buffer Texcoords
{
    vec2 TexcoordsData[];
};

layout(std430, set=2, binding = 8) buffer Tangents4
{
    vec4 Tangents4Data[];
};

layout(std430, set=2, binding = 9) buffer BoneIndices
{
    uvec4 BoneIndicesData[];
};

layout(std430, set=2, binding = 10) buffer BoneWeights
{
    vec4 BoneWeightsData[];
};

layout(std430, set=2, binding = 11) buffer DrawIndexedIndirectCommands
{
    DrawIndexedIndirectCommand DrawIndexedIndirectData[];
};

struct RenderMeshChunk
{
    uint renderObjectIndex;
    uint renderMeshPrimitiveIndex;
};

layout(std430, set=2, binding = 12) buffer RenderMeshChunks
{
    uint renderMeshChunkCount;
    RenderMeshChunk RenderMeshChunkData[];
};

struct LightCluster
{
	vec3 min;
	vec3 max;
};

layout(std430, set=2, binding = 13) buffer ViewRenderLights
{
    RenderLightSource ViewRenderLightSourceData[];
};

layout(set=2, binding=14, std430) buffer LightClustersBlock
{
	LightCluster[] clusters;
} LightClusters;

layout(set=2, binding=15, std430) buffer TileLightIndicesBlock
{
	uint[] indices;
} TileLightIndices;

layout(set=2, binding=16, std430) buffer LightClusterListsBlock
{
    uint listSize;
    uint padding;
	uvec2[] lists;
} LightClusterLists;

layout(set=2, binding=17) uniform texture2D ShadowMapAtlasTexture;

layout(set=0, binding=0) uniform sampler LinearTextureSampler;
layout(set=0, binding=1) uniform sampler NearestTextureSampler;
layout(set=0, binding=2) uniform sampler ShadowMapSampler;

layout(set=1, binding=0) uniform texture2D AllTextures[1024];

struct GuiElement
{
    uint type;
    int texture;
    float borderRoundRadius;
    float borderSize;

    vec2 rectangleMin;
    vec2 rectangleMax;
    vec2 sourceImageRectangleMin;
    vec2 sourceImageRectangleMax;

    vec4 firstColor;
    vec4 secondColor;
    vec4 borderColor;
};

layout(set=2, binding=18, std430) buffer GuiElementsBlock
{
	GuiElement[] list;
} GuiElementList;

const uint GuiElementType_SolidRectangle = 0;
const uint GuiElementType_HorizontalGradient = 1;
const uint GuiElementType_VerticalGradient = 2;
const uint GuiElementType_TextCharacter = 3;

