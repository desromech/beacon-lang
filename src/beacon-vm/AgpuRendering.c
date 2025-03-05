
#include "AgpuRendering.h"
#include <stdio.h>
#include <stdlib.h>

agpu_platform *beacon_agpu_getPlatform(beacon_context_t *context)
{
    if(context->roots.agpuPlatform)
        return beacon_unboxExternalAddress(context, context->roots.agpuPlatform);

    // Get the platform.
    agpu_uint numPlatforms;
    agpuGetPlatforms(0, NULL, &numPlatforms);
    if (numPlatforms == 0)
    {
        fprintf(stderr, "No agpu platforms are available.\n");
        return NULL;
    }
    else if(beacon_decodeSmallInteger(context->roots.agpuPlatformIndex) >= (int)numPlatforms)
    {
        fprintf(stderr, "Warning selected AGPU platform is not avaialable. Falling back to the first available\n");
        context->roots.agpuPlatformIndex = beacon_encodeSmallInteger(0);
    }

    agpu_platform **platforms = calloc(numPlatforms, sizeof(agpu_platform *));
    agpuGetPlatforms(numPlatforms, platforms, NULL);

    agpu_platform *selectedPlatform = platforms[beacon_decodeSmallInteger(context->roots.agpuPlatformIndex)];
    free(platforms);
    context->roots.agpuPlatform = beacon_boxExternalAddress(context, selectedPlatform);
    printf("Selected AGPU Platform: %s\n", agpuGetPlatformName(selectedPlatform));
    return selectedPlatform;
}

agpu_device *beacon_agpu_getDevice(beacon_context_t *context)
{
    if(context->roots.agpuDevice)
        return beacon_unboxExternalAddress(context, context->roots.agpuDevice);

    agpu_platform *platform = beacon_agpu_getPlatform(context);
    if(!platform)
        return 0;
    
    agpu_device_open_info openInfo = {};
    openInfo.gpu_index = beacon_decodeSmallInteger(context->roots.agpuDeviceIndex);
    openInfo.debug_layer = context->roots.agpuDebugLayerEnabled == context->roots.trueValue;

    agpu_device *device = agpuOpenDevice(platform, &openInfo);
    if(!device)
    {
        fprintf(stderr, "Failed to open the specified agpu device.\n");
        return NULL;
    }

    printf("Selected AGPU Device: %s\n", agpuGetDeviceName(device));
    context->roots.agpuDevice = beacon_boxExternalAddress(context, device);
    return device;
}

static beacon_oop_t beacon_AGPU_platform(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    if(!context->roots.agpuPlatform)
        beacon_agpu_getPlatform(context);
    return context->roots.agpuPlatform;
}

static beacon_oop_t beacon_AGPU_device(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    if(!context->roots.agpuDevice)
        beacon_agpu_getDevice(context);
    return context->roots.agpuDevice;
}

void beacon_context_registerAgpuRenderingPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.agpuClass), "platform", 1, beacon_AGPU_platform);
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.agpuClass), "device", 1, beacon_AGPU_device);
}

