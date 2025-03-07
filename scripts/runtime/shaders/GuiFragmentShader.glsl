
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

layout(set=0, binding=0) uniform sampler LinearTextureSampler;
layout(set=0, binding=1) uniform sampler NearestTextureSampler;
layout(set=1, binding=0) uniform texture2D GuiTextures[1024];

layout(location = 0) in vec2 inTexcoord;
layout(location = 1) flat in int inGuiElementIndex;

layout(location = 0) out vec4 outColor;

const uint GuiElementType_SolidRectangle = 0;
const uint GuiElementType_HorizontalGradient = 1;
const uint GuiElementType_VerticalGradient = 2;
const uint GuiElementType_TextCharacter = 3;

void main()
{
    GuiElement thisElement = GuiElementList.list[inGuiElementIndex];
    vec2 mappedTexcoord = mix(thisElement.sourceImageRectangleMin, thisElement.sourceImageRectangleMax, inTexcoord);
    
    vec4 backgroundColor;
    switch(thisElement.type)
    {
    case GuiElementType_SolidRectangle:
        backgroundColor = thisElement.firstColor;
        break;
    case GuiElementType_HorizontalGradient:
        backgroundColor = mix(thisElement.firstColor, thisElement.secondColor, inTexcoord.x);
        break;
    case GuiElementType_VerticalGradient:
        backgroundColor = mix(thisElement.firstColor, thisElement.secondColor, inTexcoord.y);
        break;
    case GuiElementType_TextCharacter:
        if(thisElement.texture >= 0)
        {
            float fontAlpha = texture(sampler2D(GuiTextures[thisElement.texture], NearestTextureSampler), mappedTexcoord).r;
            backgroundColor = thisElement.firstColor*fontAlpha;
        }
        else
        {
            backgroundColor = vec4(mappedTexcoord, 0.0, 1.0);
        }

        break;
    default:
        backgroundColor = vec4(inTexcoord, 0.0, 1.0);
        break;
    }

    if(thisElement.borderSize > 0)
    {
        vec2 thisPoint = mix(thisElement.rectangleMin, thisElement.rectangleMax, inTexcoord);
        vec2 interiorRectangleMin = thisElement.rectangleMin + thisElement.borderSize;
        vec2 interiorRectangleMax = thisElement.rectangleMax - thisElement.borderSize;
        bool isInInterior = interiorRectangleMin.x <= thisPoint.x && thisPoint.x <= interiorRectangleMax.x &&
                            interiorRectangleMin.y <= thisPoint.y && thisPoint.y <= interiorRectangleMax.y;

        if(!isInInterior)
            backgroundColor = thisElement.borderColor;
    }

    outColor = vec4(backgroundColor.rgb *backgroundColor.a, backgroundColor.a);    
}
