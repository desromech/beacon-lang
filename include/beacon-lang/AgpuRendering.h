#ifndef BEACON_AGPU_RENDERING_H
#define BEACON_AGPU_RENDERING_H

#include "AGPU/agpu.h"
#include "Context.h"
#include "ObjectModel.h"

typedef struct beacon_AGPU_s
{
    beacon_Object_t super;
} beacon_AGPU_t;

typedef struct beacon_AGPUSwapChain_s
{
    beacon_Object_t super;
    agpu_command_queue *commandQueue;
    agpu_swap_chain *swapChain;
} beacon_AGPUSwapChain_t;

agpu_platform *beacon_agpu_getPlatform(beacon_context_t *context);
agpu_device *beacon_agpu_getDevice(beacon_context_t *context);

#endif // BEACON_AGPU_RENDERING_H