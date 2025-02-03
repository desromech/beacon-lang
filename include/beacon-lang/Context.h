#ifndef BEACON_CONTEXT_H
#define BEACON_CONTEXT_H

#include "ObjectModel.h"
#include "Memory.h"

typedef struct beacon_context_s beacon_context_t;

struct beacon_context_s
{
    struct ContextGCRoots
    {

    } roots;
};

beacon_context_t *beacon_context_new(void);
void beacon_context_destroy(beacon_context_t *context);

#endif // BEACON_CONTEXT_H