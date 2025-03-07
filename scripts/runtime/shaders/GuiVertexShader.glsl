
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

layout(set=2, binding=11, std430) buffer GuiElementsBlock
{
	GuiElement[] list;
} GuiElementList;

layout(location = 0) out vec2 outTexcoord;
layout(location = 1) flat out int guiElementIndex;

const vec2 screenQuadVertices[4] = vec2[4](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, 1.0)
);

void main()
{
    guiElementIndex = gl_InstanceIndex;

    GuiElement thisElement = GuiElementList.list[gl_InstanceIndex];
    vec2 thisTexcoord = screenQuadVertices[gl_VertexIndex]*0.5 + 0.5;
    vec2 thisVertex = mix(thisElement.rectangleMin, thisElement.rectangleMax, thisTexcoord);
    
    vec2 thisVertexNDC = thisVertex * PushConstants.framebufferReciprocalExtent*2.0 - 1.0;
    if(!PushConstants.hasTopLeftNDCOrigin)
        thisVertexNDC * vec2(1.0, -1.0);
        
    outTexcoord = thisTexcoord;
    gl_Position = vec4(thisVertexNDC, 0.0, 1.0);
}
