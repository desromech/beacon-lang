#ifndef BEACON_SCANNER_H
#define BEACON_SCANNER_H

#pragma once

#include "ObjectModel.h"

typedef struct beacon_context_s beacon_context_t;

typedef enum beacon_TokenKind_e {
#define TokenKindName(name) BeaconToken ## name,

#include "TokenKind.inc"

#undef TokenKindName
} beacon_TokenKind_t;


beacon_OrderedCollection_t *beacon_scanSourceCode(beacon_context_t *context, beacon_SourceCode_t *sourceCode);

const char *beacon_TokenKind_toString(beacon_TokenKind_t);

#endif //BEACON_SCANNER_H