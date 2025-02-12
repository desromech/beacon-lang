#include "beacon-lang/Exceptions.h"
#include "beacon-lang/ObjectModel.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
#include <stdlib.h>
#include <stdio.h>

void beacon_exception_signal(beacon_context_t *context, beacon_Exception_t *exception)
{
    (void)context;
    size_t messageTextSize = exception->messageText->super.super.super.super.super.header.slotCount;
    fprintf(stderr, "Exception: %.*s\n", (int)messageTextSize, exception->messageText->data);
    abort();
}

void beacon_exception_error(beacon_context_t *context, const char *errorMessage)
{
    beacon_Error_t *error = beacon_allocateObjectWithBehavior(context->heap, context->classes.errorClass, sizeof(beacon_Error_t), BeaconObjectKindPointers);
    error->super.messageText = beacon_importCString(context, errorMessage);
    beacon_exception_signal(context, &error->super);
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
    snprintf(buffer, sizeof(buffer), "Subclass responsibility with implementing #%.*s", selectorSymbol->super.super.super.super.super.header.slotCount, selectorSymbol->data);
    beacon_exception_error(context, buffer);
}