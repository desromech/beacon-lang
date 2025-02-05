#include "beacon-lang/Exceptions.h"
#include <stdlib.h>
#include <stdio.h>

void beacon_exception_error(beacon_context_t *context, const char *errorMessage)
{
    (void)context;
    fprintf(stderr, "Error: %s\n", errorMessage);
    abort();
}