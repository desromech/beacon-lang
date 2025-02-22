#ifndef BEACON_OBJECT_MODEL_H
#define BEACON_OBJECT_MODEL_H

#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

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

static inline beacon_oop_t beacon_encodeSmallDoubleValue(double value)
{
    // See https://clementbera.wordpress.com/2018/11/09/64-bits-immediate-floats/ for this encoding.
    uint64_t ieee754 = 0;
    memcpy(&ieee754, &value, 4);
    uint64_t ieee754SignRotation = (ieee754 << 1) | (ieee754 >> 63) ;
    uint64_t withTag = (ieee754SignRotation << 3) | ImmediateObjectTag_SmallFloat;
    return (beacon_oop_t)withTag;
}

typedef struct beacon_context_s beacon_context_t;
typedef beacon_oop_t (*beacon_NativeCodeFunction_t)(beacon_context_t *context, beacon_oop_t receiverOrCaptures, size_t argumentCount, beacon_oop_t *arguments);

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
    beacon_Dictionary_t super;
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
    beacon_Object_t super;
    beacon_oop_t handle;
} beacon_AbstractBinaryFileStream_t;

typedef struct beacon_StdioStream_s
{
    beacon_AbstractBinaryFileStream_t super;
} beacon_StdioStream_t;

typedef struct beacon_Form_s
{
    beacon_Object_t super;
    beacon_ByteArray_t *bits;
    beacon_oop_t width;
    beacon_oop_t height;
    beacon_oop_t depth;
} beacon_Form_t;

typedef struct beacon_Window_s
{
    beacon_Object_t super;
    beacon_oop_t width;
    beacon_oop_t height;
    beacon_oop_t handle;
    beacon_oop_t drawingHandle;
} beacon_Window_t;

typedef struct beacon_WindowEvent_s
{
    beacon_Object_t super;
} beacon_WindowEvent_t;

typedef struct beacon_WindowExposeEvent_s
{
    beacon_Object_t super;
    beacon_oop_t x;
    beacon_oop_t y;
    beacon_oop_t width;
    beacon_oop_t height;
} beacon_WindowExposeEvent_t;

typedef struct beacon_WindowMouseButtonEvent_s
{
    beacon_Object_t super;
    beacon_oop_t x;
    beacon_oop_t y;
} beacon_WindowMouseButtonEvent_t;

typedef struct beacon_WindowKeyboardEvent_t
{
    beacon_Object_t super;
    beacon_oop_t scancode;
    beacon_oop_t symbol;
} beacon_WindowKeyboardEvent_t;

#ifdef __cplusplus
}
#endif

#endif // BEACON_OBJECT_MODEL_H
