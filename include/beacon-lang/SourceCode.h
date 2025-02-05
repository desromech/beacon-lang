#ifndef BEACON_LANG_SOURCE_CODE_H
#define BEACON_LANG_SOURCE_CODE_H

#pragma once

#include "ObjectModel.h"

typedef struct beacon_context_s beacon_context_t;

beacon_SourceCode_t *beacon_makeSourceCodeFromFileNamed(beacon_context_t *context, const char *fileName);
beacon_SourceCode_t *beacon_makeSourceCodeFromString(beacon_context_t *context,const char *name, const char *string);

beacon_SourcePosition_t *beacon_sourcePosition_to(beacon_context_t *context, beacon_SourcePosition_t *start, beacon_SourcePosition_t *end);
beacon_SourcePosition_t *beacon_sourcePosition_until(beacon_context_t *context, beacon_SourcePosition_t *start, beacon_SourcePosition_t *end);


#endif //BEACON_LANG_SOURCE_CODE_H