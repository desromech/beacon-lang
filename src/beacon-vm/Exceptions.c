#include "beacon-lang/Exceptions.h"
#include "beacon-lang/ObjectModel.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
#include <stdlib.h>
#include <stdio.h>

static void beacon_displayExceptionStackTrace(beacon_context_t *context);

void beacon_exception_signal(beacon_context_t *context, beacon_Exception_t *exception)
{
    (void)context;
    size_t messageTextSize = exception->messageText->super.super.super.super.super.header.slotCount;
    fprintf(stderr, "Exception: %.*s\n", (int)messageTextSize, exception->messageText->data);
    beacon_displayExceptionStackTrace(context);
    abort();
}

void beacon_exception_error(beacon_context_t *context, const char *errorMessage)
{
    beacon_Error_t *error = beacon_allocateObjectWithBehavior(context->heap, context->classes.errorClass, sizeof(beacon_Error_t), BeaconObjectKindPointers);
    error->super.messageText = beacon_importCString(context, errorMessage);
    beacon_exception_signal(context, &error->super);
}

void beacon_exception_assertionFailure(beacon_context_t *context, const char *errorMessage)
{
    beacon_AssertionFailure_t *error = beacon_allocateObjectWithBehavior(context->heap, context->classes.assertionFailureClass, sizeof(beacon_AssertionFailure_t), BeaconObjectKindPointers);
    error->super.super.messageText = beacon_importCString(context, errorMessage);
    beacon_exception_signal(context, &error->super.super);
}

void beacon_exception_scannerError(beacon_context_t *context, beacon_ScannerToken_t *token)
{
    (void)token;
    beacon_exception_error(context, "Scanning error");
}

void beacon_exception_subclassResponsibility(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector)
{
    (void)receiver;
    char buffer[256];
    beacon_Symbol_t *selectorSymbol = (beacon_Symbol_t *)selector;
    beacon_Class_t *class = (beacon_Class_t*)beacon_getClass(context, receiver);
    snprintf(buffer, sizeof(buffer), "Subclass responsibility with implementing #%.*s in %.*s.",
        selectorSymbol->super.super.super.super.super.header.slotCount, selectorSymbol->data,
        class->name->super.super.super.super.super.header.slotCount, class->name->data
    );
    beacon_exception_error(context, buffer);
}

static beacon_oop_t beacon_Exception_displayException(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    return receiver;
}

static void beacon_displaySourcePosition(beacon_context_t *context, beacon_SourcePosition_t *sourcePosition)
{
    int nameSize = sourcePosition->sourceCode->name->super.super.super.super.super.header.slotCount;
    if(sourcePosition->sourceCode->directory)
    {
        int directorySize = sourcePosition->sourceCode->directory->super.super.super.super.super.header.slotCount;
        fprintf(stderr, "%.*s%.*s:%d.%d-%d.%d\n",
            directorySize, sourcePosition->sourceCode->directory->data,
            nameSize, sourcePosition->sourceCode->name->data,
            (int)beacon_decodeSmallInteger(sourcePosition->startLine), (int)beacon_decodeSmallInteger(sourcePosition->startColumn),
            (int)beacon_decodeSmallInteger(sourcePosition->endLine), (int)beacon_decodeSmallInteger(sourcePosition->endColumn));
    }
    else
    {
        fprintf(stderr, "%.*s:%d.%d-%d.%d\n",
            nameSize, sourcePosition->sourceCode->name->data,
            (int)beacon_decodeSmallInteger(sourcePosition->startLine), (int)beacon_decodeSmallInteger(sourcePosition->startColumn),
            (int)beacon_decodeSmallInteger(sourcePosition->endLine), (int)beacon_decodeSmallInteger(sourcePosition->endColumn));
    }
}

static void beacon_displayExceptionStackTrace(beacon_context_t *context)
{
    beacon_StackFrameRecord_t *record =  beacon_getTopStackFrameRecord();
    while(record)
    {
        switch(record->kind)
        {
        case StackFrameBytecodeMethodRecord:
            beacon_displaySourcePosition(context, record->bytecodeMethodStackRecord.code->sourcePosition);
            break;
        default:
            break;
        }
        record = record->previousRecord;
    }
}

static beacon_oop_t beacon_Exception_signal(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    BeaconAssert(context, (intptr_t)argumentCount == 0);
    beacon_perform(context, receiver, (beacon_oop_t)beacon_internCString(context, "displayException"));
    fprintf(stderr, "Unhandled exception. Aborting.\n");
    beacon_displayExceptionStackTrace(context);
    abort();
    return receiver;
}

void beacon_context_registerExceptionPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, context->classes.exceptionClass, "displayException", 0, beacon_Exception_displayException);
    beacon_addPrimitiveToClass(context, context->classes.exceptionClass, "signal", 0, beacon_Exception_signal);
}
