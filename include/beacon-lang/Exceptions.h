#ifndef BEACON_LANG_EXCEPTIONS_H
#define BEACON_LANG_EXCEPTIONS_H

typedef struct beacon_context_s beacon_context_t;

void beacon_exception_error(beacon_context_t *context, const char *errorMessage);

#endif //BEACON_LANG_EXCEPTIONS_H