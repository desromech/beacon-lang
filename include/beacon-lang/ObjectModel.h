#ifndef BEACON_OBJECT_MODEL_H
#define BEACON_OBJECT_MODEL_H

#include <stdint.h>

typedef struct beacon_ObjectHeader_s beacon_ObjectHeader_t;
typedef struct beacon_Behavior_s beacon_Behavior_t;

typedef enum beacon_ObjectKind_e {
    BeaconObjectKindPointers = 0,
    BeaconObjectKindWeakPointers,
    BeaconObjectKindBytes,
} beacon_ObjectKind_t;

typedef intptr_t beacon_oop_t;

struct beacon_ObjectHeader_s
{
    uint8_t objectKind : 2;
    uint8_t gcCcolor : 2;

    uint32_t slotCount;
    beacon_Behavior_t *behavior;
};

typedef struct beacon_ProtoObject_s
{
    beacon_ObjectHeader_t header;
} beacon_ProtoObject_t;

typedef struct beacon_Object_s
{
    beacon_ProtoObject_t super;
} beacon_Object_t;

typedef struct beacon_Collection_s
{
    beacon_Object_t super;
} beacon_Collection_t;

typedef struct beacon_HashedCollection_s
{
    beacon_Collection_t super;
} beacon_HashedCollection_t;

typedef struct beacon_Dictionary_s
{
    beacon_HashedCollection_t super;
} beacon_Dictionary_t;

typedef struct beacon_MethodDictionary_s
{
    beacon_Dictionary_t super;
} beacon_MethodDictionary_t;

typedef struct beacon_Behavior_s
{
    beacon_Object_t super;
    beacon_Behavior_t *superclass;
    beacon_MethodDictionary_t methodDict;
    beacon_oop_t format;
} beacon_Behavior_t;

typedef struct beacon_ClassDescription_s
{
    beacon_Behavior_t super;
    beacon_oop_t protocols;
} beacon_ClassDescription_t;

typedef struct beacon_Class_s
{
    beacon_ClassDescription_t super;

} beacon_Class_t;

typedef struct beacon_Metaclass_s
{
    beacon_ClassDescription_t super;
    beacon_Class_t *thisClass;
} beacon_Metaclass_t;


#endif // BEACON_OBJECT_MODEL_H