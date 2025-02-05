
#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/OrderedCollection.h"

beacon_OrderedCollection_t *beacon_OrderedCollection_new(beacon_context_t *context)
{
    beacon_OrderedCollection_t *collection = beacon_allocateObjectWithBehavior(context->heap, context->roots.orderedCollectionClass, sizeof(beacon_OrderedCollection_t), BeaconObjectKindPointers);
    return collection;
}
void beacon_OrderedCollection_add(beacon_context_t *context, beacon_OrderedCollection_t *collection, beacon_oop_t element);
void beacon_OrderedCollection_size(beacon_OrderedCollection_t *collection);
beacon_oop_t beacon_OrderedCollection_at(beacon_OrderedCollection_t *collection, size_t index);