#version 450

layout( push_constant ) uniform constants
{
	bool hasTopLeftNDCOrigin;
    uint reservedConstant;
    vec2 framebufferReciprocalExtent;
} PushConstants;

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

layout(set=0, binding=0) uniform sampler LinearTextureSampler;
layout(set=0, binding=1) uniform sampler NearestTextureSampler;
layout(set=1, binding=0) uniform texture2D GuiTextures[1024];

const uint GuiElementType_SolidRectangle = 0;
const uint GuiElementType_HorizontalGradient = 1;
const uint GuiElementType_VerticalGradient = 2;
const uint GuiElementType_TextCharacter = 3;
