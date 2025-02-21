#ifndef BEACON_LANGUAGE_SYNTAX_COMPILER_H
#define BEACON_LANGUAGE_SYNTAX_COMPILER_H

#pragma once

#include "ObjectModel.h"
#include "Parser.h"
#include "Bytecode.h"

#ifdef __cplusplus
extern "C" {
#endif

beacon_CompiledMethod_t *beacon_compileFileSyntax(beacon_context_t *context, beacon_ParseTreeNode_t *parseTree, beacon_SourceCode_t *sourceCode);

#ifdef __cplusplus
}
#endif

#endif //BEACON_LANGUAGE_SYNTAX_COMPILER_H
