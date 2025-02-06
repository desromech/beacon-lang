#ifndef BEACON_LANG_PARSER_H
#define BEACON_LANG_PARSER_H

#include "ObjectModel.h"
#include "Scanner.h"

beacon_ParseTreeNode_t *beacon_parseTokenList(beacon_context_t *context, beacon_SourceCode_t *sourceCode, beacon_ArrayList_t *tokenList);

#endif //BEACON_LANG_PARSER_H
