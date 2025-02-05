#ifndef BEACON_LANG_EXCEPTIONS_H
#define BEACON_LANG_EXCEPTIONS_H

typedef struct beacon_context_s beacon_context_t;
typedef struct beacon_ScannerToken_s beacon_ScannerToken_t;

void beacon_exception_error(beacon_context_t *context, const char *errorMessage);
void beacon_exception_scannerError(beacon_context_t *context, beacon_ScannerToken_t *token);

#endif //BEACON_LANG_EXCEPTIONS_H