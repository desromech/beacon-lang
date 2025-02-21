#ifndef BEACON_LANG_PARSER_H
#define BEACON_LANG_PARSER_H

#include "ObjectModel.h"
#include "Scanner.h"

#ifdef __cplusplus
extern "C" {
#endif

beacon_ParseTreeNode_t *beacon_parseWorkspaceTokenList(beacon_context_t *context, beacon_SourceCode_t *sourceCode, beacon_ArrayList_t *tokenList);
beacon_ParseTreeNode_t *beacon_parseMethodTokenList(beacon_context_t *context, beacon_SourceCode_t *sourceCode, beacon_ArrayList_t *tokenList);

#ifdef __cplusplus
}
#endif

#endif //BEACON_LANG_PARSER_H
