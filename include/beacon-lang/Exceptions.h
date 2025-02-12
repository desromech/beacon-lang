#ifndef BEACON_LANG_EXCEPTIONS_H
#define BEACON_LANG_EXCEPTIONS_H

typedef struct beacon_context_s beacon_context_t;
typedef struct beacon_ScannerToken_s beacon_ScannerToken_t;

void beacon_exception_signal(beacon_context_t *context, beacon_Exception_t *exception);

void beacon_exception_error(beacon_context_t *context, const char *errorMessage);
void beacon_exception_assertionFailure(beacon_context_t *context, const char *errorMessage);

void beacon_exception_scannerError(beacon_context_t *context, beacon_ScannerToken_t *token);
void beacon_exception_subclassResponsibility(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector);
void beacon_exception_subclassResponsibility(beacon_context_t *context, beacon_oop_t receiver, beacon_oop_t selector);

#define BeaconAssertStringify(s) #s
#define BeaconAssert(context, x) \
    if(!(x)) \
        beacon_exception_assertionFailure(context, "Assertion Failure " #x" in " __FILE__ ":" BeaconAssertStringify(__LINE__))

#endif //BEACON_LANG_EXCEPTIONS_H