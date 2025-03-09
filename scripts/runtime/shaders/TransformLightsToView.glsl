layout(local_size_x = 128) in;

#define CameraState cameraStates[PushConstants.cameraStateIndex]

void main()
{
    uint invocationIndex = gl_GlobalInvocationID.x;
    if(invocationIndex >= PushConstants.lightSourceCount)
        return;

    RenderLightSource lightSource = WorldRenderLightSourceData[invocationIndex];
    lightSource.positionOrDirection = CameraState.viewMatrix * lightSource.positionOrDirection;
    lightSource.spotDirection = (CameraState.viewMatrix * vec4(lightSource.spotDirection, 0.0)).xyz;

    for(int i = 0; i < 4; ++i)
    {
        lightSource.modelMatrix[i] = CameraState.viewMatrix * lightSource.modelMatrix[i];
        lightSource.inverseModelMatrix[i] = lightSource.inverseModelMatrix[i] * CameraState.inverseViewMatrix;
    }

    ViewRenderLightSourceData[invocationIndex] = lightSource;
}