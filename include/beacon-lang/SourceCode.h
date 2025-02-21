#ifndef BEACON_LANG_SOURCE_CODE_H
#define BEACON_LANG_SOURCE_CODE_H

#pragma once

#include "ObjectModel.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct beacon_context_s beacon_context_t;

beacon_SourceCode_t *beacon_makeSourceCodeFromFileNamed(beacon_context_t *context, const char *fileName);
beacon_SourceCode_t *beacon_makeSourceCodeFromString(beacon_context_t *context,const char *name, const char *string);

beacon_SourcePosition_t *beacon_sourcePosition_to(beacon_context_t *context, beacon_SourcePosition_t *start, beacon_SourcePosition_t *end);
beacon_SourcePosition_t *beacon_sourcePosition_until(beacon_context_t *context, beacon_SourcePosition_t *start, beacon_SourcePosition_t *end);

beacon_oop_t beacon_evaluateSourceCode(beacon_context_t *context, beacon_SourceCode_t *sourceCode);

#ifdef __cplusplus
}
#endif

#endif //BEACON_LANG_SOURCE_CODE_H