#ifndef BEACON_OBJECT_MODEL_H
#define BEACON_OBJECT_MODEL_H

#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct beacon_ObjectHeader_s beacon_ObjectHeader_t;
typedef struct beacon_Behavior_s beacon_Behavior_t;
typedef struct beacon_String_s beacon_String_t;
typedef struct beacon_Symbol_s beacon_Symbol_t;

typedef enum beacon_ObjectKind_e {
    BeaconObjectKindPointers = 0,
    BeaconObjectKindWeakPointers,
    BeaconObjectKindBytes,
    BeaconObjectKindImmediate,
} beacon_ObjectKind_t;

typedef enum beacon_ImmediateObjectTagBits_s {
    ImmediateObjectTag_BitCount = 3,
    ImmediateObjectTag_BitMask = (1<<ImmediateObjectTag_BitCount) - 1,
    ImmediateObjectTag_SmallInteger = 1,
    ImmediateObjectTag_Character = 2,
    ImmediateObjectTag_SmallFloat = 4,
} beacon_ImmediateObjectTagBits_s;

typedef intptr_t beacon_oop_t;

static inline beacon_oop_t beacon_isImmediate(beacon_oop_t oop)
{
    return !oop || (oop & ImmediateObjectTag_BitMask) != 0;
}

static inline beacon_oop_t beacon_isSmallInteger(beacon_oop_t oop)
{
    return (oop & ImmediateObjectTag_BitMask) == ImmediateObjectTag_SmallInteger;
}

static inline beacon_oop_t beacon_isCharacter(beacon_oop_t oop)
{
    return (oop & ImmediateObjectTag_BitMask) == ImmediateObjectTag_Character;
}

static inline beacon_oop_t beacon_isSmallFloat(beacon_oop_t oop)
{
    return (oop & ImmediateObjectTag_BitMask) == ImmediateObjectTag_SmallFloat;
}

static inline intptr_t beacon_decodeSmallInteger(beacon_oop_t oop)
{
    return oop >> 3;
}

static inline beacon_oop_t beacon_encodeSmallInteger(intptr_t smallInteger)
{
    return (smallInteger << 3) | ImmediateObjectTag_SmallInteger;
}

static inline beacon_oop_t beacon_encodeCharacter(uint32_t character)
{
    return (character << 3) | ImmediateObjectTag_Character;
}

static inline uint32_t beacon_decodeCharacter(beacon_oop_t oop)
{
    return oop >> 3;
}

static inline bool beacon_isNil(beacon_oop_t oop)
{
    return oop == 0;
}

static inline bool beacon_isNotNil(beacon_oop_t oop)
{
    return oop != 0;
}

static inline beacon_oop_t beacon_encodeSmallFloat(double value)
{
    // See https://clementbera.wordpress.com/2018/11/09/64-bits-immediate-floats/ for this encoding.
    uint64_t rot = 0;
    memcpy(&rot, &value, 8);
    rot = (rot << 1) | (rot >> 63);
    if(rot > 1)
        rot -= ((uint64_t)896 << 53);
    uint64_t withTag = (rot << 3) | ImmediateObjectTag_SmallFloat;
    assert(beacon_isImmediate(withTag));
    return (beacon_oop_t)withTag;
}

static inline double beacon_decodeSmallFloat(beacon_oop_t value)
{
    assert(beacon_isImmediate(value));
 
    uint64_t rot = (uint64_t)value >> 3;
    if(rot > 1)
        rot = rot + ((uint64_t)896 << 53);

    rot = (rot >> 1) | (rot << 63);
    double decodedValue = 0;
    memcpy(&decodedValue, &rot, 8);

    return decodedValue;
}


static inline double beacon_decodeSmallNumber(intptr_t encodedNumber)
{
    intptr_t tag = encodedNumber & ImmediateObjectTag_BitMask;
    if(tag == ImmediateObjectTag_SmallInteger)
        return beacon_decodeSmallInteger(encodedNumber);
    else if(tag == ImmediateObjectTag_SmallFloat)
        return beacon_decodeSmallFloat(encodedNumber);
    else
        return 0;
}

typedef struct beacon_context_s beacon_context_t;
typedef beacon_oop_t (*beacon_NativeCodeFunction_t)(beacon_context_t *context, beacon_oop_t receiverOrCaptures, size_t argumentCount, beacon_oop_t *arguments);

double beacon_decodeNumberAsDouble(beacon_context_t *context, beacon_oop_t encodedNumber);
beacon_oop_t beacon_encodeDoubleAsNumber(beacon_context_t *context, double value);

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
    beacon_oop_t tally;
    struct beacon_Array_s *array;
} beacon_HashedCollection_t;

typedef struct beacon_Dictionary_s
{
    beacon_HashedCollection_t super;
} beacon_Dictionary_t;

typedef struct beacon_Association_s
{
    beacon_Object_t super;
    beacon_oop_t key;
    beacon_oop_t value;
} beacon_Association_t;

typedef struct beacon_MethodDictionary_s
{
    beacon_Dictionary_t super;
} beacon_MethodDictionary_t;

typedef struct beacon_NativeCode_s
{
    beacon_Object_t super;
    beacon_NativeCodeFunction_t nativeFunction;
} beacon_NativeCode_t;

typedef struct beacon_CompiledCode_s
{
    beacon_Object_t super;
    beacon_oop_t argumentCount;
    beacon_NativeCode_t *nativeImplementation;
    struct beacon_BytecodeCode_s *bytecodeImplementation;
    struct beacon_SourcePosition_s *sourcePosition;
} beacon_CompiledCode_t;

typedef struct beacon_CompiledMethod_s
{
    beacon_CompiledCode_t super;
    beacon_Symbol_t *name;
} beacon_CompiledMethod_t;

typedef struct beacon_CompiledBlock_s
{
    beacon_CompiledCode_t super;
    beacon_oop_t captureCount;
} beacon_CompiledBlock_t;

typedef struct beacon_BlockClosure_s
{
    beacon_Object_t super;
    beacon_CompiledBlock_t *code;
    beacon_oop_t captures;
} beacon_BlockClosure_t;

typedef struct beacon_Message_s
{
    beacon_Object_t super;
    beacon_oop_t selector;
    beacon_oop_t arguments;
} beacon_Message_t;

typedef struct beacon_InternedSymbolSet_s
{
    beacon_HashedCollection_t super;
} beacon_InternedSymbolSet_t;

typedef struct beacon_Slot_s
{
    beacon_Object_t super;
    beacon_oop_t name;
    beacon_oop_t index;
} beacon_Slot_t;

typedef struct beacon_Behavior_s
{
    beacon_Object_t super;
    beacon_Behavior_t *superclass;
    beacon_MethodDictionary_t *methodDict;
    beacon_oop_t instSize;
    beacon_oop_t objectKind;
    beacon_oop_t slots;
} beacon_Behavior_t;

typedef struct beacon_ClassDescription_s
{
    beacon_Behavior_t super;
    beacon_oop_t protocols;
} beacon_ClassDescription_t;

typedef struct beacon_Class_s
{
    beacon_ClassDescription_t super;
    beacon_oop_t subclasses;
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

typedef struct beacon_ByteArray_s
{
    beacon_ArrayedCollection_t super;
    uint8_t elements[];
} beacon_ByteArray_t;

typedef struct beacon_UInt16Array_s
{
    beacon_ArrayedCollection_t super;
    uint16_t elements[];
} beacon_UInt16Array_t;

typedef struct beacon_UInt32Array_s
{
    beacon_ArrayedCollection_t super;
    uint32_t elements[];
} beacon_UInt32Array_t;

typedef struct beacon_Float32Array_s
{
    beacon_ArrayedCollection_t super;
    float elements[];
} beacon_Float32Array_t;

typedef struct beacon_Float64Array_s
{
    beacon_ArrayedCollection_t super;
    double elements[];
} beacon_Float64Array_t;

typedef struct beacon_ExternalAddress_s
{
    beacon_ArrayedCollection_t super;
    void *address;
} beacon_ExternalAddress_t;

typedef struct beacon_ArrayList_s
{
    beacon_SequenceableCollection_t super;
    beacon_Array_t *array;
    beacon_oop_t size;
    beacon_oop_t capacity;
} beacon_ArrayList_t;

typedef struct beacon_ByteArrayList_s
{
    beacon_SequenceableCollection_t super;
    beacon_ByteArray_t *array;
    beacon_oop_t size;
    beacon_oop_t capacity;
} beacon_ByteArrayList_t;

typedef struct beacon_BytecodeCode_s
{
    beacon_Object_t super;
    beacon_oop_t argumentCount;
    beacon_oop_t temporaryCount;
    beacon_oop_t captureCount;
    beacon_Array_t *literals;
    beacon_ByteArray_t *bytecodes;
} beacon_BytecodeCode_t;

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

typedef struct beacon_Magnitude_s
{
    beacon_Object_t super;
} beacon_Magnitude_t;

typedef struct beacon_Number_s
{
    beacon_Magnitude_t super;
} beacon_Number_t;

typedef struct beacon_Integer_s
{
    beacon_Magnitude_t super;
} beacon_Integer_t;

typedef struct beacon_Boolean_s
{
    beacon_Object_t super;
} beacon_Boolean_t;

typedef struct beacon_True_s
{
    beacon_Boolean_t super;
} beacon_True_t;

typedef struct beacon_False_s
{
    beacon_Boolean_t super;
} beacon_False_t;

typedef struct beacon_UndefinedObject_s
{
    beacon_Object_t super;
} beacon_UndefinedObject_t;

typedef struct beacon_SmallInteger_s
{
    beacon_Integer_t super;
} beacon_SmallInteger_t;

typedef struct beacon_Character_s
{
    beacon_Magnitude_t super;
} beacon_Character_t;

typedef struct beacon_Float_s
{
    beacon_Number_t super;
} beacon_Float_t;

typedef struct beacon_SmallFloat_s
{
    beacon_Float_t super;
} beacon_SmallFloat_t;

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
    beacon_Object_t super;
    beacon_oop_t kind;
    beacon_SourcePosition_t *sourcePosition;
    beacon_String_t *errorMessage;
    beacon_oop_t textPosition;
    beacon_oop_t textSize;
} beacon_ScannerToken_t;

typedef struct beacon_ParseTreeNode_s
{
    beacon_Object_t super;
    beacon_SourcePosition_t *sourcePosition;
} beacon_ParseTreeNode_t;

typedef struct beacon_ParseTreeErrorNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_String_t *errorMessage;
    beacon_ParseTreeNode_t *innerNode;
} beacon_ParseTreeErrorNode_t;

typedef struct beacon_ParseTreeMethodNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Symbol_t *selector;
    beacon_Array_t *arguments;
    beacon_Array_t *localVariables;
    beacon_ParseTreeNode_t * expression;
} beacon_ParseTreeMethodNode_t;

typedef struct beacon_ParseTreeAddMethod_s
{
    beacon_ParseTreeNode_t super;
    beacon_ParseTreeNode_t *behavior;
    beacon_ParseTreeNode_t *method;
} beacon_ParseTreeAddMethod_t;

typedef struct beacon_ParseTreeWorkspaceScriptNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Array_t *localVariables;
    beacon_ParseTreeNode_t *expression;
} beacon_ParseTreeWorkspaceScriptNode_t;

typedef struct beacon_ParseTreeLiteralNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_oop_t value;
} beacon_ParseTreeLiteralNode_t;

typedef struct beacon_ParseTreeIdentifierReferenceNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_oop_t identifier;
} beacon_ParseTreeIdentifierReferenceNode_t;

typedef struct beacon_ParseTreeMessageSendNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_ParseTreeNode_t *receiver;
    beacon_ParseTreeNode_t *selector;
    beacon_Array_t *arguments;
} beacon_ParseTreeMessageSendNode_t;

typedef struct beacon_ParseTreeMessageCascadeNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_ParseTreeNode_t *receiver;
    beacon_Array_t *cascadedMessages;
} beacon_ParseTreeMessageCascadeNode_t;

typedef struct beacon_ParseTreeCascadedMessageNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_ParseTreeNode_t *selector;
    beacon_Array_t *arguments;
} beacon_ParseTreeCascadedMessageNode_t;

typedef struct beacon_ParseTreeSequenceNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Array_t *elements;
} beacon_ParseTreeSequenceNode_t;

typedef struct beacon_ParseTreeReturnNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_ParseTreeNode_t *expression;
} beacon_ParseTreeReturnNode_t;

typedef struct beacon_ParseTreeAssignment_s
{
    beacon_ParseTreeNode_t super;
    beacon_ParseTreeNode_t *storage;
    beacon_ParseTreeNode_t *valueToStore;
} beacon_ParseTreeAssignment_t;

typedef struct beacon_ParseTreeArgumentDefinitionNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Symbol_t *name;
} beacon_ParseTreeArgumentDefinitionNode_t;

typedef struct beacon_ParseTreeLocalVariableDefinitionNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Symbol_t *name;
} beacon_ParseTreeLocalVariableDefinitionNode_t;

typedef struct beacon_ParseTreeBlockClosureNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Array_t *arguments;
    beacon_Array_t *localVariables;
    beacon_ParseTreeNode_t *expression;
} beacon_ParseTreeBlockClosureNode_t;

typedef struct beacon_ParseTreeArrayNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Array_t *elements;
} beacon_ParseTreeArrayNode_t;

typedef struct beacon_ParseTreeByteArrayNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Array_t *elements;
} beacon_ParseTreeByteArrayNode_t;

typedef struct beacon_ParseTreeLiteralArrayNode_s
{
    beacon_ParseTreeNode_t super;
    beacon_Array_t *elements;
} beacon_ParseTreeLiteralArrayNode_t;

typedef struct beacon_BytecodeCodeBuilder_t
{
    beacon_Object_t super;
    beacon_ArrayList_t *arguments;
    beacon_ArrayList_t *temporaries;
    beacon_ArrayList_t *literals;
    beacon_ArrayList_t *captures;
    beacon_ByteArrayList_t *bytecodes;
    beacon_oop_t parentBuilder;
} beacon_BytecodeCodeBuilder_t;

typedef struct beacon_AbstractCompilationEnvironment_s
{
    beacon_Object_t super;
} beacon_AbstractCompilationEnvironment_t;

typedef struct beacon_EmptyCompilationEnvironment_s
{
    beacon_AbstractCompilationEnvironment_t super;
} beacon_EmptyCompilationEnvironment_t;

typedef struct beacon_SystemCompilationEnvironment_s
{
    beacon_AbstractCompilationEnvironment_t super;
    beacon_AbstractCompilationEnvironment_t *parent;
    beacon_MethodDictionary_t *systemDictionary;
} beacon_SystemCompilationEnvironment_t;

typedef struct beacon_FileCompilationEnvironment_s
{
    beacon_AbstractCompilationEnvironment_t super;
    beacon_AbstractCompilationEnvironment_t *parent;
    beacon_MethodDictionary_t *dictionary;
} beacon_FileCompilationEnvironment_t;

typedef struct beacon_LexicalCompilationEnvironment_s
{
    beacon_AbstractCompilationEnvironment_t super;
    beacon_AbstractCompilationEnvironment_t *parent;
    beacon_MethodDictionary_t *dictionary;
} beacon_LexicalCompilationEnvironment_t;

typedef struct beacon_BlockClosureCompilationEnvironment_s
{
    beacon_AbstractCompilationEnvironment_t super;
    beacon_AbstractCompilationEnvironment_t *parent;
    beacon_ArrayList_t *captureList;
    beacon_MethodDictionary_t *dictionary;
} beacon_BlockClosureCompilationEnvironment_t;

typedef struct beacon_MethodCompilationEnvironment_s
{
    beacon_AbstractCompilationEnvironment_t super;
    beacon_AbstractCompilationEnvironment_t *parent;
    beacon_MethodDictionary_t *dictionary;
} beacon_MethodCompilationEnvironment_t;

typedef struct beacon_BehaviorCompilationEnvironment_s
{
    beacon_AbstractCompilationEnvironment_t super;
    beacon_AbstractCompilationEnvironment_t *parent;
    beacon_Behavior_t *behavior;
} beacon_BehaviorCompilationEnvironment_t;

typedef struct beacon_Exception_s
{
    beacon_Object_t super;
    beacon_String_t *messageText;
} beacon_Exception_t;

typedef struct beacon_Abort_s
{
    beacon_Exception_t super;
} beacon_Abort_t;

typedef struct beacon_Error_s
{
    beacon_Exception_t super;
} beacon_Error_t;

typedef struct beacon_AssertionFailure_s
{
    beacon_Error_t super;
} beacon_AssertionFailure_t;

typedef struct beacon_MessageNotUnderstood_s
{
    beacon_Error_t super;
    beacon_Message_t *message;
    beacon_oop_t receiver;
    beacon_oop_t reachedDefaultHandler;
} beacon_MessageNotUnderstood_t;

typedef struct beacon_NonBooleanReceiver_s
{
    beacon_Error_t super;
} beacon_NonBooleanReceiver_t;

typedef struct beacon_UnhandledException_s
{
    beacon_Exception_t super;
} beacon_UnhandledException_t;

typedef struct beacon_UnhandledError_s
{
    beacon_UnhandledException_t super;
} beacon_UnhandledError_t;

typedef struct beacon_WeakTombstone_s
{
    beacon_Object_t super;
} beacon_WeakTombstone_t;

typedef struct beacon_Stdio_s
{
    beacon_Object_t super;
} beacon_Stdio_t;

typedef struct beacon_Stream_s
{
    beacon_Object_t super;
} beacon_Stream_t;

typedef struct beacon_AbstractBinaryFileStream_s
{
    beacon_Stream_t super;
    beacon_oop_t handle;
} beacon_AbstractBinaryFileStream_t;

typedef struct beacon_StdioStream_s
{
    beacon_AbstractBinaryFileStream_t super;
} beacon_StdioStream_t;

typedef struct beacon_Point_s
{
    beacon_Object_t super;
    beacon_oop_t x;
    beacon_oop_t y;
} beacon_Point_t;

typedef struct beacon_Color_s
{
    beacon_Object_t super;
    beacon_oop_t r;
    beacon_oop_t g;
    beacon_oop_t b;
    beacon_oop_t a;
} beacon_Color_t;

typedef struct beacon_Rectangle_s
{
    beacon_Object_t super;
    beacon_Point_t *origin;
    beacon_Point_t *corner;
} beacon_Rectangle_t;

typedef struct beacon_FormRenderingElement_s
{
    beacon_Object_t super;
    beacon_oop_t borderRoundRadius;
    beacon_oop_t borderSize;
    beacon_Rectangle_t *rectangle;
} beacon_FormRenderingElement_t;

typedef struct beacon_FormSolidRectangleRenderingElement_s
{
    beacon_FormRenderingElement_t super;
    beacon_Color_t *color;
    beacon_Color_t *borderColor;
} beacon_FormSolidRectangleRenderingElement_t;

typedef struct beacon_FormHorizontalGradientRenderingElement_s
{
    beacon_FormRenderingElement_t super;
    beacon_Color_t *startColor;
    beacon_Color_t *endColor;
    beacon_Color_t *borderColor;
} beacon_FormHorizontalGradientRenderingElement_t;

typedef struct beacon_FormVerticalGradientRenderingElement_s
{
    beacon_FormRenderingElement_t super;
    beacon_Color_t *startColor;
    beacon_Color_t *endColor;
    beacon_Color_t *borderColor;
} beacon_FormVerticalGradientRenderingElement_t;

typedef struct beacon_FormTextureHandleRenderingElement_s
{
    beacon_FormRenderingElement_t super;
    beacon_oop_t textureHandle;
} beacon_FormTextureHandleRenderingElement_t;

typedef struct beacon_Form_s
{
    beacon_Object_t super;
    beacon_ByteArray_t *bits;
    beacon_oop_t width;
    beacon_oop_t height;
    beacon_oop_t depth;
    beacon_oop_t pitch;
    beacon_oop_t textureHandle;
} beacon_Form_t;

typedef struct beacon_Font_s
{
    beacon_Object_t super;
    beacon_ByteArray_t *rawData;
} beacon_Font_t;

typedef struct beacon_FontFace_s
{
    beacon_Object_t super;
    beacon_ByteArray_t *charData;
    beacon_oop_t height;
    beacon_oop_t ascent;
    beacon_oop_t descent;
    beacon_oop_t linegap;
    beacon_Form_t *atlasForm;
    struct beacon_FontFace_s *hiDpiScaled;
} beacon_FontFace_t;

typedef struct beacon_FormTextRenderingElement_s
{
    beacon_FormRenderingElement_t super;
    beacon_String_t *text;
    beacon_Color_t *color;
    beacon_FontFace_t *fontFace;
} beacon_FormTextRenderingElement_t;

typedef struct beacon_TextureRenderingElement_s
{
    beacon_FormRenderingElement_t super;
    beacon_oop_t textureIndex;
    beacon_Rectangle_t *textureRectangle;
} beacon_TextureRenderingElement_t;

typedef struct beacon_Window_s
{
    beacon_Object_t super;
    beacon_oop_t width;
    beacon_oop_t height;
    beacon_oop_t handle;

    beacon_oop_t rendererHandle;

    beacon_oop_t textureHandle;
    beacon_oop_t textureWidth;
    beacon_oop_t textureHeight;

    beacon_oop_t drawingForm;

    beacon_oop_t useAcceleratedRendering;
    struct beacon_AGPUSwapChain_s *swapChainHandle;
} beacon_Window_t;

typedef struct beacon_WindowEvent_s
{
    beacon_Object_t super;
} beacon_WindowEvent_t;

typedef struct beacon_WindowExposeEvent_s
{
    beacon_WindowEvent_t super;
    beacon_oop_t x;
    beacon_oop_t y;
    beacon_oop_t width;
    beacon_oop_t height;
} beacon_WindowExposeEvent_t;

typedef struct beacon_WindowMouseButtonEvent_s
{
    beacon_WindowEvent_t super;
    beacon_oop_t button;
    beacon_oop_t x;
    beacon_oop_t y;
} beacon_WindowMouseButtonEvent_t;

typedef struct beacon_WindowMouseMotionEvent_s
{
    beacon_WindowEvent_t super;
    beacon_oop_t buttons;
    beacon_oop_t x;
    beacon_oop_t y;
    beacon_oop_t xrel;
    beacon_oop_t yrel;
} beacon_WindowMouseMotionEvent_t;

typedef struct beacon_WindowMouseWheelEvent_s
{
    beacon_WindowEvent_t super;
    beacon_oop_t x;
    beacon_oop_t y;
    beacon_oop_t scrollX;
    beacon_oop_t scrollY;
} beacon_WindowMouseWheelEvent_t;

typedef struct beacon_WindowKeyboardEvent_s
{
    beacon_WindowEvent_t super;
    beacon_oop_t scancode;
    beacon_oop_t symbol;
    beacon_oop_t modstate;
} beacon_WindowKeyboardEvent_t;

typedef struct beacon_WindowTextInputEvent_s
{
    beacon_WindowEvent_t super;
    beacon_oop_t text;
} beacon_WindowTextInputEvent_t;

typedef struct beacon_TestAsserter_s
{
    beacon_Object_t super;
} beacon_TestAsserter_t;

typedef struct beacon_TestCase_t
{
    beacon_TestAsserter_t super;
} beacon_TestCase_t;

typedef struct beacon_AbstractPrimitiveTensor_s
{
    beacon_Object_t super;
} beacon_AbstractPrimitiveTensor_t;

typedef struct beacon_AbstractPrimitiveMatrix_s
{
    beacon_AbstractPrimitiveTensor_t super;
} beacon_AbstractPrimitiveMatrix_t;

typedef struct beacon_Matrix2x2_s
{
    beacon_AbstractPrimitiveMatrix_t super;
    double m11; double m21;
    double m12; double m22;
} beacon_Matrix2x2_t;

typedef struct beacon_Matrix3x3_s
{
    beacon_AbstractPrimitiveMatrix_t super;
    double m11; double m21; double m31;
    double m12; double m22; double m32;
    double m13; double m23; double m33;
} beacon_Matrix3x3_t;

typedef struct beacon_Matrix4x4_s
{
    beacon_AbstractPrimitiveMatrix_t super;
    double m11; double m21; double m31; double m41;
    double m12; double m22; double m32; double m42;
    double m13; double m23; double m33; double m43;
    double m14; double m24; double m34; double m44;
} beacon_Matrix4x4_t;

typedef struct beacon_AbstractPrimitiveVector_s
{
    beacon_AbstractPrimitiveTensor_t super;
} beacon_AbstractPrimitiveVector_t;

typedef struct beacon_Vector2_s
{
    beacon_AbstractPrimitiveVector_t super;
    double x;
    double y;
} beacon_Vector2_t;

typedef struct beacon_Vector3_s
{
    beacon_AbstractPrimitiveVector_t super;
    double x;
    double y;
    double z;
} beacon_Vector3_t;

typedef struct beacon_Vector4_s
{
    beacon_AbstractPrimitiveVector_t super;
    double x;
    double y;
    double z;
    double w;
} beacon_Vector4_t;

typedef struct beacon_Complex_s
{
    beacon_Object_t super;
    double x;
    double y;
} beacon_Complex_t;

typedef struct beacon_Quaternion_s
{
    beacon_Object_t super;
    double x;
    double y;
    double z;
    double w;
} beacon_Quaternion_t;

#ifdef __cplusplus
}
#endif

#endif // BEACON_OBJECT_MODEL_H
