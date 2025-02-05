#include "beacon-lang/Exceptions.h"
#include "beacon-lang/ObjectModel.h"
#include <stdlib.h>
#include <stdio.h>

void beacon_exception_error(beacon_context_t *context, const char *errorMessage)
{
    (void)context;
    fprintf(stderr, "Error: %s\n", errorMessage);
    abort();
}

void beacon_exception_scannerError(beacon_context_t *context, beacon_ScannerToken_t *token)
{
    (void)token;
    beacon_exception_error(context, "Scanning error");
}