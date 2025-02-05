#ifndef BEACON_OBJECT_MODEL_H
#define BEACON_OBJECT_MODEL_H

#include <stdint.h>

typedef struct beacon_ObjectHeader_s beacon_ObjectHeader_t;
typedef struct beacon_Behavior_s beacon_Behavior_t;
typedef struct beacon_String_s beacon_String_t;
typedef struct beacon_Symbol_s beacon_Symbol_t;


typedef enum beacon_ObjectKind_e {
    BeaconObjectKindPointers = 0,
    BeaconObjectKindWeakPointers,
    BeaconObjectKindBytes,
} beacon_ObjectKind_t;

typedef enum beacon_ImmediateObjectTagBits_s {
    ImmediateObjectTag_BitCount = 3,
    ImmediateObjectTag_BitMask = (1<<ImmediateObjectTag_BitCount) - 1,
    ImmediateObjectTag_SmallInteger = 1,
} beacon_ImmediateObjectTagBits_s;

typedef intptr_t beacon_oop_t;

static inline beacon_oop_t beacon_isImmediate(beacon_oop_t oop)
{
    return (oop & ImmediateObjectTag_BitMask) != 0;
}

static inline beacon_oop_t beacon_encodeSmallInteger(intptr_t smallInteger)
{
    return (smallInteger << 3) | ImmediateObjectTag_SmallInteger;
}

static inline intptr_t beacon_decodeSmallInteger(beacon_oop_t oop)
{
    return oop >> 3;
}

struct beacon_ObjectHeader_s
{
    uint8_t objectKind : 2;
    uint8_t gcColor : 2;

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
    beacon_Symbol_t *name;

} beacon_Class_t;

typedef struct beacon_Metaclass_s
{
    beacon_ClassDescription_t super;
    beacon_Class_t *thisClass;
} beacon_Metaclass_t;

typedef struct beacon_SequenceableCollection_s
{
    beacon_Collection_t super;
} beacon_SequenceableCollection_t;

typedef struct beacon_ArrayedCollection_s
{
    beacon_SequenceableCollection_t super;
} beacon_ArrayedCollection_t;

typedef struct beacon_Array_s
{
    beacon_ArrayedCollection_t super;
    beacon_oop_t elements[];
} beacon_Array_t;

typedef struct beacon_OrderedCollection_s
{
    beacon_SequenceableCollection_t super;
    beacon_Array_t *array;
    beacon_oop_t firstIndex;
    beacon_oop_t lastIndex;
} beacon_OrderedCollection_t;

typedef struct beacon_String_s
{
    beacon_ArrayedCollection_t super;
    uint8_t data[];
} beacon_String_t;

typedef struct beacon_Symbol_s
{
    beacon_ArrayedCollection_t super;
    uint8_t data[];
} beacon_Symbol_t;

typedef struct beacon_SourceCode_s
{
    beacon_Object_t super;
    beacon_String_t *directory;
    beacon_String_t *name;
    beacon_String_t *text;
    beacon_oop_t textSize;
}beacon_SourceCode_t;

typedef struct beacon_SourcePosition_s
{
    beacon_Object_t super;
    beacon_SourceCode_t *sourceCode;
    beacon_oop_t startIndex;
    beacon_oop_t endIndex;
    beacon_oop_t startLine;
    beacon_oop_t endLine;
    beacon_oop_t startColumn;
    beacon_oop_t endColumn;
} beacon_SourcePosition_t;

typedef struct beacon_ScannerToken_s
{
    beacon_oop_t kind;
    beacon_SourcePosition_t *sourcePosition;
    beacon_String_t *text;
    beacon_String_t *errorMessage;
    beacon_oop_t textPosition;
    beacon_oop_t textSize;
} beacon_ScannerToken_t;

#endif // BEACON_OBJECT_MODEL_H